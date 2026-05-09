#pragma once

#include <G4Track.hh>
#include <G4UserEventAction.hh>
#include <string>
#include <unordered_map>

#include "Configs.hh"

class RunAction;
class G4Step;

class EventAction: public G4UserEventAction {
  public:
    EventAction(RunAction* runAction, GCConfig config, int updatePeriod);
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    void RegisterPrimary(int trackID, std::string primaryName, int pdg);
    std::string GetPrimaryName(const G4Track* track) const;
    int GetPrimaryPDG(const G4Track* track) const;

    void SetSoilSourceProperties(double z, double rho, double energy, int pdg);
    double GetSoilSourceZ() const;
    double GetSoilSourceRho() const;
    double GetSoilSourceEnergy() const;
    int GetSoilSourcePDG() const;

    void SetDetectionProperties(
        double energyInSoil,
        int sourcePDG,
        double zInSoil,
        double rhoInSoil
    );
    double GetDetectionStartingEnergyInSoil() const;
    int GetDetectionSourcePDG() const;
    double GetDetectionSoilZ() const;
    double GetDetectionSoilRho() const;

    void DepositEnergy(double eDep);
    double GetDetectionPulseEnergy() const;

  private:
    RunAction* fRunAction;
    GCConfig config;
    int updatePeriod;
    std::unordered_map<int, std::string> primariesMap;
    std::unordered_map<int, int> pdgsMap;

    // Only used in GCConfig::Transport
    double soilSourceZ = 0.0;
    double soilSourceRho = 0.0;
    double soilSourceEnergy = 0.0;
    int soilSourcePDG = 0;

    // Only used in GCConfig::Detect
    double detectionEnergyInSoil = 0.0;
    int detectionSourcePDG = 0;
    double detectionZInSoil = 0.0;
    double detectionRhoInSoil = 0.0;
    double detectionPulseEnergy = 0.0;
};