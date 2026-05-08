#include <G4UserTrackingAction.hh>

#include "EventAction.hh"

class TrackingAction: public G4UserTrackingAction {
  public:
    TrackingAction(EventAction* eventAction);
    virtual void PreUserTrackingAction(const G4Track* track);

  private:
    EventAction* eventAction;
};