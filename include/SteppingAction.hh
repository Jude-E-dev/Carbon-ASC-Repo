#pragma once

#include <G4UserSteppingAction.hh>

#include "Configs.hh"

class SteppingAction: public G4UserSteppingAction
{
  public:
    SteppingAction(const GCConfig& config);
    void UserSteppingAction(const G4Step* step) override;

  private:
    GCConfig config;
};