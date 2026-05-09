#pragma once

#include <G4String.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <fstream>
#include <random>

#include "Configs.hh"

class G4Event;
class G4ParticleTable;
class G4ParticleGun;

/**
 * @brief Primary generator for replaying particles during transport runs
 * 
 */
class TransportPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction
{
  public:
    /**
   * @brief Construct a new Transport Primary Generator Action object
   * 
   * @param distributionPath Path to the binary file containing soil gammas
   * @param soil Configuration information about the soil
   */
    TransportPrimaryGeneratorAction(
        const std::string& distributionPath,
        const SoilConfig& soil
    );
    ~TransportPrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event* anEvent);

    /**
     * @brief Get the amount of time that would need to pass to achieve this
     * detection, in seconds
     * 
     * @return G4double Exposure time, in seconds
     */
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