#pragma once

#include <G4VUserPrimaryGeneratorAction.hh>
#include <fstream>
#include <random>

class G4Event;
class G4ParticleTable;
class G4ParticleGun;

class ReplayPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction {
  public:
    ReplayPrimaryGeneratorAction(
        const std::string& distributionPath,
        double soilWidth
    );
    ~ReplayPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);

  private:
    G4ParticleTable* particleTable;
    G4ParticleGun* gun;
    std::ifstream binFile;
    std::mt19937 rng;
    std::uniform_real_distribution<double> azimuthDist;
    std::uniform_real_distribution<double> positionDist;
};