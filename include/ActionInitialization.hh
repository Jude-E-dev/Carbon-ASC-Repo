#pragma once

#include <G4VUserActionInitialization.hh>

#include "GCConfig.hh"

class G4GeneralParticleSource;

class ActionInitialization: public G4VUserActionInitialization {
  public:
    ActionInitialization(GCConfig mode);
    virtual ~ActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;

  private:
    GCConfig config;
};