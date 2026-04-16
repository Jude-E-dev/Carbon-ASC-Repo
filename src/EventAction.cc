#include "EventAction.hh"

#include <G4Event.hh>
#include <G4Types.hh>
#include <G4ios.hh>

#include "RunAction.hh"

EventAction::EventAction(RunAction* runAction, GCConfig config) :
    G4UserEventAction(),
    config {config} {}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event* ev) {}

void EventAction::EndOfEventAction(const G4Event* ev) {
    G4int n = ev->GetEventID();
    if (n % 1000 == 0) {
        G4cout << "\n"
               << ev->GetEventID() / 1000 << "k events completed."
               << std::flush;
    }
}