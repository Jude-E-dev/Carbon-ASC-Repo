#pragma once

#include <G4String.hh>
#include <G4Types.hh>

#include "CRYGenerator.h"
#include "CRYParticle.h"
#include "CRYPrimaryGeneratorMessenger.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "vector"

class G4Event;

class CRYPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction
{
  public:
    CRYPrimaryGeneratorAction(const char* filename);
    ~CRYPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);
    void InputCRY();
    void UpdateCRY(G4String* MessInput);
    void CRYFromFile(G4String newValue);
    G4double GetExposureTime() const;

  private:
    std::vector<CRYParticle*>* vect; // vector of generated particles
    G4ParticleTable* particleTable;
    G4ParticleGun* particleGun;
    CRYGenerator* gen;
    G4int InputState;
    CRYPrimaryGeneratorMessenger* gunMessenger;
};