#pragma once

#include "G4VModularPhysicsList.hh"
#include "globals.hh"

class GCPhysicsList: public G4VModularPhysicsList {
  public:
    GCPhysicsList(G4bool highPrecision, G4int ver = 1);
    virtual ~GCPhysicsList() = default;

    // delete copy constructor and assignment operator
    GCPhysicsList(const GCPhysicsList&) = delete;
    GCPhysicsList& operator=(const GCPhysicsList& right) = delete;
};