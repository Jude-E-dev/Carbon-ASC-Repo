#pragma once

#include <G4UserRunAction.hh>
#include <fstream>

#include "Configs.hh"
#include "GammaEntry.hh"

class G4Run;

class RunAction: public G4UserRunAction {
  public:
    RunAction(GCConfig config);
    ~RunAction();

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
    void TallyGammaBinary(double energy, double momentumZ, double depth, int pdg);

  private:
    std::chrono::steady_clock::time_point startTime;
    GCConfig config;

    // Only used in GCConfig::CharacteriseSoilBinary
    std::vector<GammaEntry> binEntries;
    std::ofstream binFile;
};