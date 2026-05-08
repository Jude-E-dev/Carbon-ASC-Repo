#include "SteppingAction.hh"

#include <G4AnalysisManager.hh>
#include <G4ProcessManager.hh>
#include <G4RunManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4Track.hh>
#include <G4ios.hh>

#include "EventAction.hh"
#include "GCConstants.hh"
#include "RunAction.hh"
#include "overloaded.hh"

SteppingAction::SteppingAction(GCConfig config, EventAction* eventAction) :
    config {config},
    eventAction {eventAction} {}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    std::visit(
        overloaded {
            [this, step](const CharacteriseSoilDebug& conf) {
                G4Track* track = step->GetTrack();

                // skip primaries
                const G4VProcess* creator = track->GetCreatorProcess();
                if (!creator)
                    return;

                // only record gamma secondaries on their first step
                G4int stepNum = track->GetCurrentStepNumber();
                const G4ParticleDefinition* particleDef =
                    track->GetParticleDefinition();

                if (particleDef->GetPDGEncoding() != GAMMA_PDG || stepNum != 1)
                    return;

                G4StepPoint* point = step->GetPreStepPoint();
                G4ThreeVector pos = point->GetPosition();
                if (pos.getZ() > 0.0)
                    return;

                auto analysisManager = G4AnalysisManager::Instance();

                // Energy
                G4double energy = point->GetKineticEnergy();
                // Skip low energy emissions
                if (energy < 0.3 * MeV)
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
                analysisManager->FillNtupleSColumn(
                    i++,
                    creator->GetProcessName()
                );
                analysisManager->FillNtupleDColumn(i++, track->GetGlobalTime());
                analysisManager->FillNtupleSColumn(
                    i++,
                    eventAction->GetPrimaryName(track)
                );

                analysisManager->AddNtupleRow();

                // Kill the secondary; tertiaries will be generated during replay.
                track->SetTrackStatus(fStopAndKill);
            },
            [this, step](const CharacteriseSoilBinary& conf) {
                G4Track* track = step->GetTrack();

                // skip primaries
                if (!track->GetCreatorProcess())
                    return;

                // only record secondaries on their first step
                if (track->GetParticleDefinition()->GetPDGEncoding()
                        != GAMMA_PDG
                    || track->GetCurrentStepNumber() != 1)
                    return;

                G4StepPoint* point = step->GetPreStepPoint();
                G4double depth = point->GetPosition().getZ();
                if (depth > 0.0)
                    return;

                // Energy
                G4double energy = point->GetKineticEnergy();
                // Skip low energy emissions
                if (energy < 0.3 * MeV)
                    return;

                RunAction* runAction = (RunAction*)G4RunManager::GetRunManager()
                                           ->GetUserRunAction();

                runAction->TallyGammaBinary(
                    energy,
                    point->GetMomentumDirection().getZ(),
                    depth,
                    eventAction->GetPrimaryPDG(track)
                );
            },
            [](auto const&) {}
        },
        config
    );
}