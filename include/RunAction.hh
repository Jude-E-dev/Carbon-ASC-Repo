#pragma once

#include <G4UserRunAction.hh>
#include <fstream>

#include "GCConfig.hh"
#include "GammaEntry.hh"

class G4Run;

class RunAction: public G4UserRunAction {
  public:
    RunAction(GCConfig config);
    ~RunAction();

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
    void TallyGammaBin(double energy, double momentumZ, double depth);

  private:
    std::chrono::steady_clock::time_point startTime;
    GCConfig config;

    // Only used in GCConfig::CharacteriseSoilBinary
    std::vector<GammaEntry> binEntries;
    std::ofstream binFile;
};