#include "SensitiveDetector.hh"

#include <G4AnalysisManager.hh>
#include <G4RunManager.hh>
#include <G4TrackStatus.hh>
#include <G4VSensitiveDetector.hh>
#include <G4ios.hh>

#include "EventAction.hh"

SensitiveDetector::SensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name)
{
}

void SensitiveDetector::Initialize(G4HCofThisEvent* hc) {}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hc) {}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* ROhist)
{
    const auto* runManager = G4RunManager::GetRunManager();
    auto* eventAction = (EventAction*)runManager->GetUserEventAction();
    eventAction->DepositEnergy(step->GetTotalEnergyDeposit());

    return true;
}