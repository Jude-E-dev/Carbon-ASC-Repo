#pragma once

#include "G4VModularPhysicsList.hh"

/**
 * @brief The simplified physics list for this GroundCarbon project
 * 
 */
class GCPhysicsList: public G4VModularPhysicsList
{
  public:
    GCPhysicsList(G4int ver = 1);
    GCPhysicsList(GCPhysicsList&&) = delete;
    GCPhysicsList& operator=(GCPhysicsList&&) = delete;
    ~GCPhysicsList() override = default;

    GCPhysicsList(const GCPhysicsList&) = delete;
    GCPhysicsList& operator=(const GCPhysicsList& right) = delete;
};