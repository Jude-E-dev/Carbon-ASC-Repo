#include "SteppingAction.hh"

#include <G4AnalysisManager.hh>
#include <G4Gamma.hh>
#include <G4LogicalVolume.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleMomentum.hh>
#include <G4ProcessManager.hh>
#include <G4RunManager.hh>
#include <G4Step.hh>
#include <G4StepPoint.hh>
#include <G4StepStatus.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4Track.hh>
#include <G4VProcess.hh>
#include <G4ios.hh>

#include "Configs.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "overloaded.hh"

namespace
{
constexpr G4double MIN_RECORDING_ENERGY = 0.3 * MeV;
}

SteppingAction::SteppingAction(
    const GCConfig& config,
    EventAction* eventAction,
    RunAction* runAction
)
    : config {config}
    , eventAction {eventAction}
    , runAction {runAction}
{
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    auto visitor = overloaded {
        [step, this](const CharacteriseSoilGammas& conf) {
            auto* track = step->GetTrack();

            // skip primaries
            const G4VProcess* creator = track->GetCreatorProcess();
            if (!creator) {
                return;
            }

            const G4int stepNum = track->GetCurrentStepNumber();
            const auto* particleDef = track->GetParticleDefinition();

            // only record gamma secondaries on their first step
            if (particleDef->GetPDGEncoding()
                    != G4Gamma::GammaDefinition()->GetPDGEncoding()
                || stepNum != 1) {
                return;
            }

            const auto* point = step->GetPreStepPoint();
            const G4ThreeVector pos = point->GetPosition();

            if (pos.getZ() > 0.0) {
                return;
            }

            const G4double energy = point->GetKineticEnergy();
            const G4ThreeVector momentumDir = point->GetMomentumDirection();

            // Skip low energy emissions
            if (energy < MIN_RECORDING_ENERGY) {
                return;
            }

            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                updateSoilCSV(
                    energy,
                    pos,
                    momentumDir,
                    *creator,
                    track->GetGlobalTime(),
                    eventAction->GetPrimaryName(*track)
                );
            }

            // For binary file
            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
                runAction->TallySoilGammaBinary(
                    energy,
                    pos.getZ(),
                    momentumDir.getZ(),
                    eventAction->GetPrimaryPDG(*track)
                );
            }

            // Kill the secondary; tertiaries will be generated during transport.
            track->SetTrackStatus(fStopAndKill);
        },
        [step, this](const TransportGammas& conf) {
            auto* track = step->GetTrack();
            const auto* post = step->GetPostStepPoint();
            const auto* pre = step->GetPreStepPoint();
            const auto* preVol = pre->GetTouchableHandle()->GetVolume();
            const auto* postVol = post->GetTouchableHandle()->GetVolume();

            if (!preVol || !postVol) {
                return;
            }

            const G4String preName = preVol->GetName();
            const G4String postName = postVol->GetName();

            if (!(preName != "PhysicalDetector"
                  && postName == "PhysicalDetector")) {
                return;
            }

            const auto pos = pre->GetPosition();
            const auto momentum = pre->GetMomentumDirection();
            const auto* particleDef = track->GetParticleDefinition();
            const G4double energy = pre->GetKineticEnergy();

            // For csv file
            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                updateTransportCSV(
                    energy,
                    particleDef->GetParticleName(),
                    pos,
                    momentum
                );
            }

            // For binary file
            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
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

void SteppingAction::updateSoilCSV(
    G4double energy,
    const G4ThreeVector& pos,
    const G4ThreeVector& momentumDir,
    const G4VProcess& creator,
    G4double time,
    const G4String& primaryName
)
{
    G4int col = 0;
    auto* analysisManager = G4AnalysisManager::Instance();

    // Energy
    analysisManager->FillNtupleDColumn(col++, energy);

    // Momentum
    analysisManager->FillNtupleDColumn(col++, momentumDir.getX());
    analysisManager->FillNtupleDColumn(col++, momentumDir.getY());
    analysisManager->FillNtupleDColumn(col++, momentumDir.getZ());

    // Position
    analysisManager->FillNtupleDColumn(col++, pos.getX());
    analysisManager->FillNtupleDColumn(col++, pos.getY());
    analysisManager->FillNtupleDColumn(col++, pos.getZ());

    // Metadata
    analysisManager->FillNtupleSColumn(col++, creator.GetProcessName());
    analysisManager->FillNtupleDColumn(col++, time);
    analysisManager->FillNtupleSColumn(col++, primaryName);

    analysisManager->AddNtupleRow();
}

void SteppingAction::updateTransportCSV(
    G4double energy,
    const G4String& particleName,
    const G4ThreeVector& pos,
    const G4ThreeVector& momentum
)
{
    G4int col = 0;
    auto* analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillNtupleDColumn(col++, energy);
    analysisManager->FillNtupleSColumn(col++, particleName);
    analysisManager->FillNtupleDColumn(col++, pos.getZ());
    analysisManager->FillNtupleDColumn(col++, pos.getRho());
    analysisManager->FillNtupleDColumn(col++, pos.getPhi());
    analysisManager->FillNtupleDColumn(col++, momentum.getZ());
    analysisManager->FillNtupleDColumn(col++, momentum.getRho());
    analysisManager->FillNtupleDColumn(col++, momentum.getPhi());
    analysisManager->FillNtupleDColumn(col++, eventAction->GetSoilSourceZ());
    analysisManager->FillNtupleDColumn(col++, eventAction->GetSoilSourceRho());
    analysisManager->FillNtupleDColumn(
        col++,
        eventAction->GetSoilSourceEnergy()
    );
    analysisManager->FillNtupleIColumn(col++, eventAction->GetSoilSourcePDG());
    analysisManager->AddNtupleRow();
}