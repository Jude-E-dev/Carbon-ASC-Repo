#include <G4UserTrackingAction.hh>

#include "EventAction.hh"

/**
 * @brief Actions taken when categorising tracks
 * 
 */
class TrackingAction: public G4UserTrackingAction
{
  public:
    TrackingAction(EventAction* eventAction);

    void PreUserTrackingAction(const G4Track* track) override;

  private:
    EventAction* eventAction;
};