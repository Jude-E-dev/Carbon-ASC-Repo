#include <G4UserTrackingAction.hh>

/**
 * @brief Actions taken when categorising tracks
 * 
 */
class TrackingAction: public G4UserTrackingAction
{
  public:
    virtual void PreUserTrackingAction(const G4Track* track);
};