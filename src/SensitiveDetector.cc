#include "SensitiveDetector.hh"

#include <G4AnalysisManager.hh>
#include <G4RunManager.hh>
#include <G4VSensitiveDetector.hh>
#include <G4ios.hh>

#include "EventAction.hh"

SensitiveDetector::SensitiveDetector(const G4String& name) :
    G4VSensitiveDetector(name) {}

void SensitiveDetector::Initialize(G4HCofThisEvent* hc) {}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hc) {}

G4bool
SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* ROhist) {
    G4Track* track = step->GetTrack();

    const G4ParticleDefinition* particleDef = track->GetParticleDefinition();

    G4StepPoint* point = step->GetPreStepPoint();

    double eDep = step->GetTotalEnergyDeposit();

    auto analysisManager = G4AnalysisManager::Instance();
    auto* eventAction =
        (EventAction*)(G4RunManager::GetRunManager()->GetUserEventAction());
    eventAction->DepositEnergy(eDep);
    int i = 0;
    analysisManager->FillNtupleDColumn(0, i++, point->GetKineticEnergy());
    analysisManager->FillNtupleDColumn(0, i++, eDep);
    analysisManager->FillNtupleSColumn(0, i++, particleDef->GetParticleName());
    analysisManager->FillNtupleDColumn(0, i++, eventAction->GetSourceDepth());
    analysisManager->FillNtupleDColumn(0, i++, eventAction->GetSourceRadius());
    analysisManager->FillNtupleDColumn(0, i++, eventAction->GetSourceEnergy());
    analysisManager->FillNtupleIColumn(0, i++, eventAction->GetSourcePDG());
    analysisManager->AddNtupleRow(0);

    return true;
}