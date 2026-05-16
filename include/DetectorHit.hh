
#include <G4THitsCollection.hh>
#include <G4Types.hh>
#include <G4VHit.hh>

class DetectorHit: public G4VHit
{
  public:
    void SetEdep(G4double eDep);
    G4double GetEdep() const;

  private:
    G4double eDep = 0.0;
};