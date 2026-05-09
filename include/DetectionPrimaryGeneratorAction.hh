#pragma once

#include <G4Types.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <fstream>
#include <random>

class G4Event;
class G4ParticleTable;
class G4ParticleGun;

class DetectionPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction
{
  public:
    DetectionPrimaryGeneratorAction(const std::string& distributionPath);
    ~DetectionPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* ev);
    G4double GetExposureTime() const;

  private:
    G4ParticleTable* particleTable;
    G4ParticleGun* gun;
    std::ifstream binFile;
    std::mt19937 rng;
    std::uniform_real_distribution<G4double> azimuthDist;
    unsigned long eventsProcessed;
    unsigned long binFileNumEvents;
    G4double binFileExposure;
};