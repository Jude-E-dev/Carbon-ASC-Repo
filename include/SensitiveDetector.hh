#pragma once

#include <G4THitsCollection.hh>
#include <G4VSensitiveDetector.hh>

#include "DetectorHit.hh"
#include "EventAction.hh"

/**
 * @brief Called as particles step through the detector. Note that this is only
 * used in 'detect' runs.
 * 
 */
class SensitiveDetector: public G4VSensitiveDetector
{
  public:
    SensitiveDetector(const G4String& name);
    void Initialize(G4HCofThisEvent* hce) override;

  protected:
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* /*unused*/) override;

  private:
    G4THitsCollection<DetectorHit>* hitsCollection = nullptr;
    G4int hcID = -1;
};