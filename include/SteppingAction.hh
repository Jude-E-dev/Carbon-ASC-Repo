#pragma once

#include <G4UserSteppingAction.hh>

#include "Configs.hh"

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
    SteppingAction(const GCConfig& config);
    void UserSteppingAction(const G4Step* step) override;

  private:
    GCConfig config;
};