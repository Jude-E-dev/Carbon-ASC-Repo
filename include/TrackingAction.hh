#include <G4UserTrackingAction.hh>

class TrackingAction: public G4UserTrackingAction {
  public:
    virtual void PreUserTrackingAction(const G4Track* track);
};