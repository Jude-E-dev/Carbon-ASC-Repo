#pragma once

#include <G4String.hh>
#include <G4Track.hh>
#include <G4UserEventAction.hh>
#include <unordered_map>

#include "Configs.hh"

class RunAction;
class G4Step;

class EventAction: public G4UserEventAction
{
  public:
    EventAction(const GCConfig& config, G4int updatePeriod);
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    void RegisterPrimary(G4int trackID, const G4String& primaryName, G4int pdg);
    G4String GetPrimaryName(const G4Track* track) const;
    G4int GetPrimaryPDG(const G4Track* track) const;

    void SetSoilSourceProperties(
        G4double z,
        G4double rho,
        G4double energy,
        G4int pdg
    );
    G4double GetSoilSourceZ() const;
    G4double GetSoilSourceRho() const;
    G4double GetSoilSourceEnergy() const;
    G4int GetSoilSourcePDG() const;

    void SetDetectionProperties(
        G4double energyInSoil,
        G4int sourcePDG,
        G4double zInSoil,
        G4double rhoInSoil
    );
    G4double GetDetectionStartingEnergyInSoil() const;
    G4int GetDetectionSourcePDG() const;
    G4double GetDetectionSoilZ() const;
    G4double GetDetectionSoilRho() const;

    void DepositEnergy(G4double eDep);
    G4double GetDetectionPulseEnergy() const;

  private:
    GCConfig config;
    G4int updatePeriod;
    std::unordered_map<G4int, G4String> primariesMap;
    std::unordered_map<G4int, G4int> pdgsMap;

    // Only used in GCConfig::Transport
    G4double soilSourceZ = 0.0;
    G4double soilSourceRho = 0.0;
    G4double soilSourceEnergy = 0.0;
    G4int soilSourcePDG = 0;

    // Only used in GCConfig::Detect
    G4double detectionEnergyInSoil = 0.0;
    G4int detectionSourcePDG = 0;
    G4double detectionZInSoil = 0.0;
    G4double detectionRhoInSoil = 0.0;
    G4double detectionPulseEnergy = 0.0;
};