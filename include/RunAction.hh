#pragma once

#include <G4UserRunAction.hh>
#include <fstream>

#include "BinaryEntries.hh"
#include "Configs.hh"

class G4Run;

class RunAction: public G4UserRunAction {
  public:
    RunAction(GCConfig config);
    ~RunAction();

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void TallySoilGammaBinary(
        double energy,
        double momentumZ,
        double z,
        int pdg
    );
    void TallyDetectorSurfaceBinary(
        int pdg,
        double z,
        double rho,
        double energy,
        double momentumZ,
        double momentumRho,
        double momentumPhiDiff,
        int sourcePDG,
        double energyInSoil,
        double zInSoil,
        double rhoInSoil
    );

    void IncrementTransportExposure(double dt);
    void IncrementDetectionExposure(double dt);
    double GetTransportExposure() const;
    double GetDetectionExposure() const;

  private:
    std::chrono::steady_clock::time_point startTime;
    GCConfig config;

    // Only used in GCConfig::CharacteriseSoil binary
    std::vector<TransportGammaEntry> soilBinEntries;
    std::ofstream soilBinFile;

    // Only used in GCConfig::Transport
    double transportExposure = 0.0;
    std::vector<DetectorSurfaceEntry> detectorSurfaceBinEntries;
    std::ofstream detectorSurfaceBinFile;

    // Only used in GCConfig::Detect
    double detectionExposure = 0.0;
};