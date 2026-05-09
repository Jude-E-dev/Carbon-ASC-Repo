#pragma once

#include <G4VSensitiveDetector.hh>

/**
 * @brief Called as particles step through the detector. Note that this is only
 * used in 'detect' runs.
 * 
 */
class SensitiveDetector: public G4VSensitiveDetector
{
  public:
    SensitiveDetector(const G4String& name);
    void Initialize(G4HCofThisEvent* hc);
    void EndOfEvent(G4HCofThisEvent* hc);

  protected:
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist);
};