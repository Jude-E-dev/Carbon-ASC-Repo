#include "SteppingAction.hh"

#include <G4AnalysisManager.hh>
#include <G4ProcessManager.hh>
#include <G4Track.hh>

#define GAMMA_PDG 22

void SteppingAction::UserSteppingAction(const G4Step* step) {
    G4Track* track = step->GetTrack();

    // skip primaries
    const G4VProcess* creator = track->GetCreatorProcess();
    if (!creator)
        return;

    // only record secondaries on their first step
    G4int stepNum = track->GetCurrentStepNumber();
    const G4ParticleDefinition* particleDef = track->GetParticleDefinition();

    if (particleDef->GetPDGEncoding() != GAMMA_PDG || stepNum != 1)
        return;

    G4StepPoint* point = step->GetPreStepPoint();
    G4ThreeVector pos = point->GetPosition();
    if (pos.getZ() > 0.0)
        return;

    auto analysisManager = G4AnalysisManager::Instance();

    // Energy
    G4double energy = point->GetKineticEnergy();
    if (energy < 0.3)
        return;

    G4int i = 0;
    analysisManager->FillNtupleDColumn(i++, energy);

    // Momentum
    G4ThreeVector momentumDir = point->GetMomentumDirection();
    analysisManager->FillNtupleDColumn(i++, momentumDir.getX());
    analysisManager->FillNtupleDColumn(i++, momentumDir.getY());
    analysisManager->FillNtupleDColumn(i++, momentumDir.getZ());

    // Position
    analysisManager->FillNtupleDColumn(i++, pos.getX());
    analysisManager->FillNtupleDColumn(i++, pos.getY());
    analysisManager->FillNtupleDColumn(i++, pos.getZ());

    // Metadata
    analysisManager->FillNtupleSColumn(i++, creator->GetProcessName());
    analysisManager->FillNtupleDColumn(i++, track->GetGlobalTime());

    analysisManager->AddNtupleRow();

    // Kill the secondary; tertiaries will be generated during replay.
    track->SetTrackStatus(fStopAndKill);
}