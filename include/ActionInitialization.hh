#pragma once

#include <G4VUserActionInitialization.hh>

#include "Configs.hh"

class G4GeneralParticleSource;

class ActionInitialization: public G4VUserActionInitialization {
  public:
    ActionInitialization(
        SoilConfig soil,
        GCConfig mode,
        int updatePeriod = 1000
    );
    virtual ~ActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;

  private:
    GCConfig config;
    SoilConfig soil;
    int updatePeriod;
};