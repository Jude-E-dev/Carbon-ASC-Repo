#pragma once

#include "CRYGenerator.h"
#include "CRYParticle.h"
#include "CRYPrimaryGeneratorMessenger.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "vector"

class G4Event;

class CRYPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction {
  public:
    CRYPrimaryGeneratorAction(const char* filename);
    ~CRYPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);
    void InputCRY();
    void UpdateCRY(std::string* MessInput);
    void CRYFromFile(G4String newValue);
    double GetExposureTime();

  private:
    std::vector<CRYParticle*>* vect; // vector of generated particles
    G4ParticleTable* particleTable;
    G4ParticleGun* particleGun;
    CRYGenerator* gen;
    G4int InputState;
    CRYPrimaryGeneratorMessenger* gunMessenger;
};