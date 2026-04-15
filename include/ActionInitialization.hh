#pragma once

#include <G4VUserActionInitialization.hh>

#include "GCMode.hh"

class G4GeneralParticleSource;

class ActionInitialization: public G4VUserActionInitialization {
  public:
    ActionInitialization(GCMode mode);
    virtual ~ActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;

  private:
    GCMode mode;
};