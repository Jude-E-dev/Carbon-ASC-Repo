#pragma once

#include <G4VSensitiveDetector.hh>

class SensitiveDetector: public G4VSensitiveDetector {
  public:
    SensitiveDetector(const G4String& name);
    void Initialize(G4HCofThisEvent* hc);
    void EndOfEvent(G4HCofThisEvent* hc);

  protected:
    G4bool ProcessHits(G4Step* aStep, G4TouchableHistory* ROhist);
};