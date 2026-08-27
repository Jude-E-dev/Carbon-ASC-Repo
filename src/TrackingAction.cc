#include "TrackingAction.hh"

#include <G4RunManager.hh>
#include <G4Track.hh>
#include <G4Types.hh>
#include <G4ios.hh>

#include "EventAction.hh"

TrackingAction::TrackingAction(EventAction* eventAction)
    : eventAction {eventAction}
{
}

void TrackingAction::PreUserTrackingAction(const G4Track* track)
{
    const G4int parentID = track->GetParentID();
    if (parentID == 0) { // primaries
        const G4int trackID = track->GetTrackID();
        const auto* particleDef = track->GetParticleDefinition();
        eventAction->RegisterPrimary(
            trackID,
            particleDef->GetParticleName(),
            particleDef->GetPDGEncoding()
        );
    }
    else {
        eventAction->RegisterPrimary(
            track->GetTrackID(),
            eventAction->GetPrimaryName(*track),
            eventAction->GetPrimaryPDG(*track)
        );
    }
}
