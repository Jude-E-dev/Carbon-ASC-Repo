#pragma once

#include <G4VUserPrimaryGeneratorAction.hh>

class G4Event;
class G4ParticleTable;
class G4ParticleGun;

class ReplayPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction {
  public:
    ReplayPrimaryGeneratorAction();
    ~ReplayPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);

  private:
    G4ParticleTable* particleTable;
    G4ParticleGun* gun;
};