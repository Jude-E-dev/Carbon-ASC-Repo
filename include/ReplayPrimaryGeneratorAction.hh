#pragma once

#include <G4VUserPrimaryGeneratorAction.hh>
#include <fstream>
#include <random>

#include "Configs.hh"

class G4Event;
class G4ParticleTable;
class G4ParticleGun;

class ReplayPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction {
  public:
    ReplayPrimaryGeneratorAction(
        const std::string& distributionPath,
        SoilConfig soil
    );
    ~ReplayPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);
    double GetExposureTime();

  private:
    SoilConfig soil;
    G4ParticleTable* particleTable;
    G4ParticleGun* gun;
    std::ifstream binFile;
    std::mt19937 rng;
    std::uniform_real_distribution<double> azimuthDist;
    std::uniform_real_distribution<double> positionDist;
    unsigned long eventsProcessed;
    unsigned long binFileNumEvents;
    double binFileExposure;
};