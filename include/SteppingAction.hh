#pragma once

#include <G4ThreeVector.hh>
#include <G4UserSteppingAction.hh>

#include "Configs.hh"
#include "EventAction.hh"
#include "RunAction.hh"

/**
 * @brief Called as particles step through the world volume.
 * 
 */
class SteppingAction: public G4UserSteppingAction
{
  public:
    /**
   * @brief Construct a new Stepping Action object
   * 
   * @param config Configuration information for the whole run
   */
    SteppingAction(
        const GCConfig& config,
        EventAction* eventAction,
        RunAction* runAction
    );
    void UserSteppingAction(const G4Step* step) override;

  private:
    void updateSoilCSV(
        G4double energy,
        const G4ThreeVector& pos,
        const G4ThreeVector& momentumDir,
        const G4VProcess& creator,
        G4double time,
        const G4String& primaryName
    );

    void updateTransportCSV(
        G4double energy,
        const G4String& particleName,
        const G4ThreeVector& pos,
        const G4ThreeVector& momentum
    );

    GCConfig config;
    EventAction* eventAction;
    RunAction* runAction;
};