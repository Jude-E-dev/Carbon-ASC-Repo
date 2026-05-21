#pragma once

#include <G4String.hh>
#include <G4Types.hh>
#include <string_view>

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
    CRYPrimaryGeneratorAction(std::string_view filename, std::string_view cryDataPath);

    ~CRYPrimaryGeneratorAction() override = default;
    CRYPrimaryGeneratorAction(const CRYPrimaryGeneratorAction&) = default;
    CRYPrimaryGeneratorAction(CRYPrimaryGeneratorAction&&) = delete;
    CRYPrimaryGeneratorAction& operator=(const CRYPrimaryGeneratorAction&) = default;
    CRYPrimaryGeneratorAction& operator=(CRYPrimaryGeneratorAction&&) = delete;

    void GeneratePrimaries(G4Event* event) override;
    void InputCRY();
    void UpdateCRY(G4String* input);
    void CRYFromFile(const G4String& newPath);
    G4double GetExposureTime() const;

  private:
    std::vector<CRYParticle*> particles; // vector of generated particles
    G4ParticleTable* particleTable;
    G4ParticleGun* particleGun;
    CRYGenerator* gen;
    G4int inputState;
    CRYPrimaryGeneratorMessenger* gunMessenger;

    G4String cryDataPath;
};