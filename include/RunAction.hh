#pragma once

#include <G4UserRunAction.hh>
#include <fstream>

#include "BinaryEntries.hh"
#include "Configs.hh"

class G4Run;

class RunAction: public G4UserRunAction
{
  public:
    RunAction(const GCConfig& config);
    ~RunAction();

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void TallySoilGammaBinary(
        G4double energy,
        G4double momentumZ,
        G4double z,
        G4int pdg
    );
    void TallyDetectorSurfaceBinary(
        G4int pdg,
        G4double z,
        G4double rho,
        G4double energy,
        G4double momentumZ,
        G4double momentumRho,
        G4double momentumPhiDiff,
        G4int sourcePDG,
        G4double energyInSoil,
        G4double zInSoil,
        G4double rhoInSoil
    );

    void IncrementTransportExposure(G4double dt);
    void IncrementDetectionExposure(G4double dt);
    G4double GetTransportExposure() const;
    G4double GetDetectionExposure() const;

  private:
    std::chrono::steady_clock::time_point startTime;
    GCConfig config;

    // Only used in GCConfig::CharacteriseSoil binary
    std::vector<TransportGammaEntry> soilBinEntries;
    std::ofstream soilBinFile;

    // Only used in GCConfig::Transport
    G4double transportExposure = 0.0;
    std::vector<PreDetectorEntry> preDetectorBinEntries;
    std::ofstream preDetectorBinFile;

    // Only used in GCConfig::Detect
    G4double detectionExposure = 0.0;
};