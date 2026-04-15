#pragma once

#include <G4UserRunAction.hh>

#include "GCMode.hh"

class G4Run;

class RunAction: public G4UserRunAction {
  public:
    RunAction(GCMode mode);
    ~RunAction();

  private:
    std::chrono::steady_clock::time_point startTime;
    GCMode mode;

  public:
    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
};