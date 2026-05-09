#pragma once

#include "G4VModularPhysicsList.hh"

class GCPhysicsList: public G4VModularPhysicsList
{
  public:
    GCPhysicsList(G4int ver = 1);
    virtual ~GCPhysicsList() = default;

    GCPhysicsList(const GCPhysicsList&) = delete;
    GCPhysicsList& operator=(const GCPhysicsList& right) = delete;
};