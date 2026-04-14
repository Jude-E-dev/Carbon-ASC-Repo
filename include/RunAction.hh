#pragma once

// #include <G4String.hh>
#include <G4UserRunAction.hh>

class G4Run;
// class RunActionMessenger;
// class GCDetectorConstruction;

class RunAction: public G4UserRunAction {
  public:
    RunAction();
    ~RunAction();

  private:
    std::chrono::steady_clock::time_point startTime;

  public:
    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;
};