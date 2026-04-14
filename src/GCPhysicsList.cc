
#include "GCPhysicsList.hh"

#include <G4DecayPhysics.hh>
#include <G4EmExtraPhysics.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4HadronElasticPhysicsHP.hh>
#include <G4HadronPhysicsFTFP_BERT.hh> // Very Fast.
#include <G4HadronPhysicsFTFP_BERT_HP.hh> // Slow.
// #include <G4HadronPhysicsQGSP_BERT.hh>  // Fast?
// #include <G4HadronPhysicsQGSP_BERT_HP.hh> // Slow.
// #include <G4HadronPhysicsQGSP_BIC.hh> // Very Fast.
// #include <G4HadronPhysicsQGSP_BIC_HP.hh> // Slow.
#include <G4IonElasticPhysics.hh>
#include <G4IonPhysics.hh>
#include <G4RadioactiveDecayPhysics.hh>
#include <G4StoppingPhysics.hh>

GCPhysicsList::GCPhysicsList(G4bool highPrecision, G4int ver) {
    defaultCutValue = 7 * CLHEP::mm;
    SetCutValue(0, "proton");
    SetVerboseLevel(ver);

    // EM Physics
    RegisterPhysics(new G4EmStandardPhysics_option4(ver));

    // Synchroton Radiation & GN Physics
    RegisterPhysics(new G4EmExtraPhysics(ver));

    // Decays
    RegisterPhysics(new G4DecayPhysics(ver));
    RegisterPhysics(new G4RadioactiveDecayPhysics(ver));

    // Problem children - Hadron Elastic scattering and Physics
    // RegisterPhysics(new G4HadronElasticPhysicsHP(ver));
    if (highPrecision) {
        RegisterPhysics(new G4HadronPhysicsFTFP_BERT_HP(ver));
    }
    else {
        RegisterPhysics(new G4HadronPhysicsFTFP_BERT(ver));
    }

    // Stopping Physics
    RegisterPhysics(new G4StoppingPhysics(ver));

    // Ion Physics
    RegisterPhysics(new G4IonElasticPhysics(ver));
    RegisterPhysics(new G4IonPhysics(ver));
}