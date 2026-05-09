#include "SteppingAction.hh"

#include <G4AnalysisManager.hh>
#include <G4LogicalVolume.hh>
#include <G4ProcessManager.hh>
#include <G4RunManager.hh>
#include <G4Step.hh>
#include <G4StepPoint.hh>
#include <G4StepStatus.hh>
#include <G4SystemOfUnits.hh>
#include <G4Track.hh>
#include <G4ios.hh>

#include "Configs.hh"
#include "EventAction.hh"
#include "GCConstants.hh"
#include "RunAction.hh"
#include "overloaded.hh"

SteppingAction::SteppingAction(GCConfig config) : config {config} {}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    auto visitor = overloaded {
        [step](const CharacteriseSoilGammas& conf) {
            G4Track* track = step->GetTrack();

            // skip primaries
            const G4VProcess* creator = track->GetCreatorProcess();
            if (!creator)
                return;

            G4int stepNum = track->GetCurrentStepNumber();
            const G4ParticleDefinition* particleDef =
                track->GetParticleDefinition();

            // only record gamma secondaries on their first step
            if (particleDef->GetPDGEncoding() != GAMMA_PDG || stepNum != 1)
                return;

            G4StepPoint* point = step->GetPreStepPoint();
            G4ThreeVector pos = point->GetPosition();

            if (pos.getZ() > 0.0)
                return;

            auto analysisManager = G4AnalysisManager::Instance();
            auto* runManager = G4RunManager::GetRunManager();
            auto* eventAction = (EventAction*)runManager->GetUserEventAction();
            G4double energy = point->GetKineticEnergy();
            G4ThreeVector momentumDir = point->GetMomentumDirection();

            // Skip low energy emissions
            if (energy < 0.3 * MeV)
                return;

            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                G4int i = 0;
                // Energy
                analysisManager->FillNtupleDColumn(i++, energy);

                // Momentum
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

                // For csv file
                analysisManager->FillNtupleSColumn(
                    i++,
                    eventAction->GetPrimaryName(track)
                );

                analysisManager->AddNtupleRow();
            }

            // For binary file
            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
                auto* runAction = (RunAction*)runManager->GetUserRunAction();
                runAction->TallySoilGammaBinary(
                    energy,
                    momentumDir.getZ(),
                    pos.getZ(),
                    eventAction->GetPrimaryPDG(track)
                );
            }

            // Kill the secondary; tertiaries will be generated during transport.
            track->SetTrackStatus(fStopAndKill);
        },
        [step](const TransportGammas& conf) {
            const auto* post = step->GetPostStepPoint();
            const auto* pre = step->GetPreStepPoint();
            const auto* preVol = pre->GetTouchableHandle()->GetVolume();
            const auto* postVol = post->GetTouchableHandle()->GetVolume();

            if (!preVol || !postVol)
                return;

            const auto preName = preVol->GetName();
            const auto postName = postVol->GetName();

            if (!(preName != "PhysicalDetector"
                  && postName == "PhysicalDetector"))
                return;

            auto* track = step->GetTrack();
            const auto* runManager = G4RunManager::GetRunManager();
            const auto* eventAction =
                (EventAction*)runManager->GetUserEventAction();

            const auto pos = pre->GetPosition();
            const auto momentum = pre->GetMomentumDirection();
            const auto* particleDef = track->GetParticleDefinition();
            const double energy = pre->GetKineticEnergy();

            // For csv file
            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                auto* analysisManager = G4AnalysisManager::Instance();

                int i = 0;
                analysisManager->FillNtupleDColumn(i++, energy);
                analysisManager->FillNtupleSColumn(
                    i++,
                    particleDef->GetParticleName()
                );
                analysisManager->FillNtupleDColumn(i++, pos.getZ());
                analysisManager->FillNtupleDColumn(i++, pos.getRho());
                analysisManager->FillNtupleDColumn(i++, pos.getPhi());
                analysisManager->FillNtupleDColumn(i++, momentum.getZ());
                analysisManager->FillNtupleDColumn(i++, momentum.getRho());
                analysisManager->FillNtupleDColumn(i++, momentum.getPhi());
                analysisManager->FillNtupleDColumn(
                    i++,
                    eventAction->GetSoilSourceZ()
                );
                analysisManager->FillNtupleDColumn(
                    i++,
                    eventAction->GetSoilSourceRho()
                );
                analysisManager->FillNtupleDColumn(
                    i++,
                    eventAction->GetSoilSourceEnergy()
                );
                analysisManager->FillNtupleIColumn(
                    i++,
                    eventAction->GetSoilSourcePDG()
                );
                analysisManager->AddNtupleRow();
            }

            // For binary file
            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
                auto* runAction = (RunAction*)runManager->GetUserRunAction();
                runAction->TallyDetectorSurfaceBinary(
                    particleDef->GetPDGEncoding(),
                    pos.getZ(),
                    pos.getRho(),
                    energy,
                    momentum.getZ(),
                    momentum.getRho(),
                    momentum.getPhi() - pos.getPhi(),
                    eventAction->GetSoilSourcePDG(),
                    eventAction->GetSoilSourceEnergy(),
                    eventAction->GetSoilSourceZ(),
                    eventAction->GetSoilSourceRho()
                );
            }

            track->SetTrackStatus(fStopAndKill);
        },
        [](const auto&) {}
    };

    std::visit(visitor, config);
}