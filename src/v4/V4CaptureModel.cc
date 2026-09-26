#include "v4/V4CaptureModel.hh"

#include <G4Deuteron.hh>
#include <G4DynamicParticle.hh>
#include <G4Gamma.hh>
#include <G4HadProjectile.hh>
#include <G4LorentzVector.hh>
#include <G4Neutron.hh>
#include <G4NeutronHPCapture.hh>
#include <G4Nucleus.hh>
#include <G4PhysicsModelCatalog.hh>
#include <G4Proton.hh>
#include <G4RandomDirection.hh>
#include <G4SystemOfUnits.hh>

#include <cmath>

V4CaptureModel::V4CaptureModel()
    : G4HadronicInteraction("V4CaptureModel")
    , hp {new G4NeutronHPCapture()}
    , secondaryModelID {G4PhysicsModelCatalog::GetModelID("model_NeutronHPCapture")}
{
    SetMinEnergy(0.0);
    SetMaxEnergy(20.0 * MeV);
    hp->SetMinEnergy(0.0);
    hp->SetMaxEnergy(20.0 * MeV);
}

V4CaptureModel::~V4CaptureModel() = default; // hp is owned by the model registry

void V4CaptureModel::BuildPhysicsTable(const G4ParticleDefinition& particle)
{
    hp->BuildPhysicsTable(particle);
}

void V4CaptureModel::InitialiseModel()
{
    hp->InitialiseModel();
}

const std::pair<G4double, G4double> V4CaptureModel::GetFatalEnergyCheckLevels() const
{
    return hp->GetFatalEnergyCheckLevels();
}

G4HadFinalState* V4CaptureModel::ApplyYourself(const G4HadProjectile& projectile,
                                               G4Nucleus& target)
{
    if (target.GetZ_asInt() != 1 || target.GetA_asInt() != 1) {
        return hp->ApplyYourself(projectile, target);
    }

    // n + p (at rest) -> d + gamma, isotropic in the centre-of-mass frame.
    hydrogenResult.Clear();
    const G4double mn = G4Neutron::Definition()->GetPDGMass();
    const G4double mp = G4Proton::Definition()->GetPDGMass();
    const G4double md = G4Deuteron::Definition()->GetPDGMass();
    const G4LorentzVector total =
        projectile.Get4Momentum() + G4LorentzVector(0.0, 0.0, 0.0, mp);
    const G4double s = total.m2();
    const G4double eGammaCM = (s - md * md) / (2.0 * std::sqrt(s));
    const G4ThreeVector direction = G4RandomDirection();
    G4LorentzVector gamma(eGammaCM * direction, eGammaCM);
    G4LorentzVector deuteron(-eGammaCM * direction, std::sqrt(md * md + eGammaCM * eGammaCM));
    const G4ThreeVector boost = total.boostVector();
    gamma.boost(boost);
    deuteron.boost(boost);

    hydrogenResult.SetStatusChange(stopAndKill);
    hydrogenResult.AddSecondary(
        new G4DynamicParticle(G4Gamma::Definition(), gamma.vect().unit(), gamma.e()),
        secondaryModelID
    );
    hydrogenResult.AddSecondary(
        new G4DynamicParticle(G4Deuteron::Definition(), deuteron.vect().unit(),
                              deuteron.e() - md),
        secondaryModelID
    );
    return &hydrogenResult;
}
