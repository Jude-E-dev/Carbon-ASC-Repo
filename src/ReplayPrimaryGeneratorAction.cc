#include "ReplayPrimaryGeneratorAction.hh"

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleTable.hh>

ReplayPrimaryGeneratorAction::ReplayPrimaryGeneratorAction() : gun(0) {
    particleTable = G4ParticleTable::GetParticleTable();
    gun = new G4ParticleGun();
}

ReplayPrimaryGeneratorAction::~ReplayPrimaryGeneratorAction() {
    delete gun;
}

void ReplayPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev) {
    

    gun->GeneratePrimaryVertex(ev);
}
