#pragma once

#include <G4UserEventAction.hh>

#include "GCConfig.hh"

class RunAction;
class G4Step;

class EventAction: public G4UserEventAction {
  public:
    EventAction(RunAction* runAction, GCConfig config);
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

  private:
    RunAction* fRunAction;
    GCConfig config;
};
