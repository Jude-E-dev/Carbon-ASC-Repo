#pragma once

#include <G4UserSteppingAction.hh>

#include "GCConfig.hh"

class SteppingAction: public G4UserSteppingAction {
  public:
    SteppingAction(GCConfig config);
    void UserSteppingAction(const G4Step* step) override;

  private:
    GCConfig config;
};