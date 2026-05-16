#include "DetectorHit.hh"

#include <G4Types.hh>

void DetectorHit::SetEdep(G4double eDep)
{
    this->eDep = eDep;
}

G4double DetectorHit::GetEdep() const
{
    return eDep;
}