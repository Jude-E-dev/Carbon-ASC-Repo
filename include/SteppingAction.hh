#pragma once

#include <G4UserSteppingAction.hh>

#include "Configs.hh"
#include "EventAction.hh"

class SteppingAction: public G4UserSteppingAction {
  public:
    SteppingAction(GCConfig config, EventAction* eventAction);
    void UserSteppingAction(const G4Step* step) override;

  private:
    GCConfig config;
    EventAction* eventAction;
};