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
    /**
   * @brief Construct a new Detection Primary Generator Action object
   * 
   * @param distributionPath Path to the binary file from a soil
   * characterisation run.
   */
    DetectionPrimaryGeneratorAction(const std::string& distributionPath);
    ~DetectionPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* ev);
    /**
     * @brief Get the amount of time that would need to pass to achieve this
     * detection, in seconds
     * 
     * @return G4double Exposure time, in seconds
     */
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