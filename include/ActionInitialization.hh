#pragma once

#include <G4VUserActionInitialization.hh>

#include "Configs.hh"

class G4GeneralParticleSource;

class ActionInitialization: public G4VUserActionInitialization
{
  public:
    /**
   * @brief Construct a new Action Initialization object
   * 
   * @param soil Configuration information for the soil
   * @param config Configuration information for the whole run
   * @param updatePeriod Print progress updates every `updatePeriod` events
   */
    ActionInitialization(
        const SoilConfig& soil,
        const GCConfig& config,
        G4int updatePeriod = 1000
    );
    virtual ~ActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;

  private:
    GCConfig config;
    SoilConfig soil;
    G4int updatePeriod;
};