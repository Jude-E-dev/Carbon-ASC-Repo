#include "v4/V4PhysicsList.hh"

#include <G4DecayPhysics.hh>
#include <G4EmExtraPhysics.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4HadronElasticPhysicsHP.hh>
#include <G4HadronPhysicsFTFP_BERT_HP.hh>
#include <G4HadronPhysicsQGSP_BIC_HP.hh>
#include <G4HadronicInteraction.hh>
#include <G4HadronicProcess.hh>
#include <G4IonElasticPhysics.hh>
#include <G4IonPhysics.hh>
#include <G4Neutron.hh>
#include <G4ProcessManager.hh>
#include <G4RadioactiveDecayPhysics.hh>
#include <G4StoppingPhysics.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThermalNeutrons.hh>
#include <G4ios.hh>

#include <stdexcept>

#include "v4/V4CaptureModel.hh"

V4PhysicsList::V4PhysicsList(
    const std::string& hadronic,
    bool thermalScattering,
    const std::string& captureModel
)
    : captureModel {captureModel}
{
    const G4int ver = 0;
    defaultCutValue = 1.0 * mm;
    SetCutValue(0, "proton");
    SetVerboseLevel(ver);

    RegisterPhysics(new G4EmStandardPhysics_option4(ver));
    RegisterPhysics(new G4EmExtraPhysics(ver));
    RegisterPhysics(new G4DecayPhysics(ver));
    RegisterPhysics(new G4RadioactiveDecayPhysics(ver));
    RegisterPhysics(new G4HadronElasticPhysicsHP(ver));
    if (hadronic == "QGSP_BIC_HP") {
        RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP(ver));
    }
    else {
        RegisterPhysics(new G4HadronPhysicsFTFP_BERT_HP(ver));
    }
    RegisterPhysics(new G4StoppingPhysics(ver));
    RegisterPhysics(new G4IonElasticPhysics(ver));
    RegisterPhysics(new G4IonPhysics(ver));
    if (thermalScattering) {
        RegisterPhysics(new G4ThermalNeutrons(ver));
    }
}

void V4PhysicsList::ConstructProcess()
{
    G4VModularPhysicsList::ConstructProcess();
    if (captureModel != "hp") {
        return;
    }

    constexpr G4double HP_MAX = 20.0 * CLHEP::MeV;
    auto* processes = G4Neutron::Definition()->GetProcessManager()->GetProcessList();
    G4HadronicProcess* capture = nullptr;
    for (std::size_t i = 0; i < processes->size(); i++) {
        auto* hadronic = dynamic_cast<G4HadronicProcess*>((*processes)[i]);
        if (hadronic != nullptr && hadronic->GetProcessSubType() == fCapture) {
            capture = hadronic;
        }
    }
    if (capture == nullptr) {
        throw std::runtime_error("V4PhysicsList: no neutron capture process found");
    }
    // Take the HP range away from whatever final-state models cover it: a
    // model reaching above 20 MeV keeps its upper part, one entirely below is
    // moved to an unreachable window. Then register the data-driven model.
    for (auto* model : capture->GetHadronicInteractionList()) {
        if (model->GetMinEnergy() >= HP_MAX) {
            continue;
        }
        G4cout << "V4PhysicsList: capture model " << model->GetModelName() << " ["
               << model->GetMinEnergy() / CLHEP::MeV << ", "
               << model->GetMaxEnergy() / CLHEP::MeV << "] MeV";
        if (model->GetMaxEnergy() > HP_MAX) {
            model->SetMinEnergy(HP_MAX);
            G4cout << " -> now above " << HP_MAX / CLHEP::MeV << " MeV only" << G4endl;
        }
        else {
            model->SetMinEnergy(1.0e9 * CLHEP::TeV);
            model->SetMaxEnergy(2.0e9 * CLHEP::TeV);
            G4cout << " -> retired" << G4endl;
        }
    }
    capture->RegisterMe(new V4CaptureModel());
    G4cout << "V4PhysicsList: neutron capture below 20 MeV uses V4CaptureModel "
              "(G4NeutronHPCapture; exact n+p->d+gamma for H-1)" << G4endl;
}
