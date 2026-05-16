#pragma once

#include <G4UserRunAction.hh>
#include <fstream>

#include "BinaryEntries.hh"
#include "Configs.hh"

class G4Run;

/**
 * @brief Code that runs per-run (across many events)
 * 
 */
class RunAction: public G4UserRunAction
{
  public:
    RunAction(const GCConfig& config);

    RunAction(const RunAction&) = delete;
    RunAction(RunAction&&) = delete;
    RunAction& operator=(const RunAction&) = delete;
    RunAction& operator=(RunAction&&) = delete;
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run* /*unused*/) override;
    void EndOfRunAction(const G4Run* /*unused*/) override;

    /**
     * @brief Tally information during a characterisation event. These will all
     * be stored in a binary file if the program was run with 'binary'
     * 
     * @param energy The energy of the gamma ray produced
     * @param momentumZ The z-momentum of the gamma ray produced
     * @param z The depth of the gamma ray produced
     * @param pdg The PDG of the cosmic ray that this gamma ray traces back to
     */
    void TallySoilGammaBinary(
        G4double energy,
        G4double momentumZ,
        G4double depth,
        G4int pdg
    );

    /**
     * @brief Tally information during a transport event. These will all be
     * stored in a binary file if the program was run with 'binary'. This
     * should be called when a particle is about to intersect the detector.
     * 
     * @param pdg The PDG of the particle incident to the detector
     * @param z The z-position of the particle incident to the detector
     * @param rho The distance from the z-axis of the particle incident to the
     * detector
     * @param energy The energy of the particle incident to the detector
     * @param momentumZ The z-momentum of the particle incident to the detector
     * @param momentumRho The radial momentum of the particle incident to the
     * detector
     * @param momentumPhiDiff The angle of the momentum of the particle
     * incident to the detector, measured relative to the particle's position
     * azimuth
     * @param sourcePDG The PDG of the cosmic ray that led to this event
     * @param energyInSoil The energy of the gamma ray that led to this event,
     * when it was produced in the soil
     * @param zInSoil The depth of the gamma ray that led to this event,
     * when it was produced in the soil
     * @param rhoInSoil The distance from the z-axis of the gamma ray that led
     * to this event, when it was produced in the soil
     */
    void TallyDetectorSurfaceBinary(
        G4int pdg,
        G4double depth,
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

    void IncrementTransportExposure(G4double deltaTime);
    void IncrementDetectionExposure(G4double deltaTime);
    G4double GetTransportExposure() const;
    G4double GetDetectionExposure() const;

  private:
    void writeSoilEntries(G4double exposure);
    void writePreDetectorEntries(G4double exposure);

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