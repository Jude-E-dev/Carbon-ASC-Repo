#pragma once

#include <G4String.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <fstream>
#include <random>

#include "Configs.hh"

class G4Event;
class G4ParticleTable;
class G4ParticleGun;

class TransportPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction
{
  public:
    TransportPrimaryGeneratorAction(
        const std::string& distributionPath,
        const SoilConfig& soil
    );
    ~TransportPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);
    G4double GetExposureTime() const;

  private:
    SoilConfig soil;
    G4ParticleTable* particleTable;
    G4ParticleGun* gun;
    std::ifstream binFile;
    std::mt19937 rng;
    std::uniform_real_distribution<G4double> azimuthDist;
    std::uniform_real_distribution<G4double> positionDist;
    unsigned long eventsProcessed;
    unsigned long binFileNumEvents;
    G4double binFileExposure;
};