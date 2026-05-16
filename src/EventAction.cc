#include "EventAction.hh"

#include <G4AnalysisManager.hh>
#include <G4Event.hh>
#include <G4RunManager.hh>
#include <G4SDManager.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>
#include <G4THitsCollection.hh>
#include <G4Track.hh>
#include <G4Types.hh>
#include <G4ios.hh>
#include <variant>

#include "Configs.hh"
#include "SensitiveDetector.hh"
#include "TransportPrimaryGeneratorAction.hh"

EventAction::EventAction(const GCConfig& config, G4int updatePeriod)
    : config {config}
    , updatePeriod {updatePeriod}
{
}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event* /*unused*/)
{
    primariesMap.clear();
    pdgsMap.clear();
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    G4int eventNum = event->GetEventID();
    if (eventNum % updatePeriod == 0) {
        G4cout << event->GetEventID() / 1000 << "k events completed." << G4endl;
    }

    if (std::holds_alternative<Detect>(config)) {
        auto* analysisManager = G4AnalysisManager::Instance();
        auto* sdm = G4SDManager::GetSDMpointer();
        G4int hcID = sdm->GetCollectionID("MeasurementSD/hits");
        auto* hce = event->GetHCofThisEvent();
        if (hce == nullptr) {
            return;
        }
        auto* hitsCollection =
            dynamic_cast<G4THitsCollection<DetectorHit>*>(hce->GetHC(hcID));
        if (hitsCollection == nullptr) {
            return;
        }

        G4double eDep = 0.0;
        for (std::size_t i = 0; i < hitsCollection->GetSize(); ++i) {
            eDep += (*hitsCollection)[i]->GetEdep();
        }

        G4int col = 0;
        if (eDep > 1.0 * keV) {
            analysisManager->FillNtupleDColumn(col++, eDep);
            analysisManager->FillNtupleDColumn(
                col++,
                GetDetectionStartingEnergyInSoil()
            );
            analysisManager->FillNtupleIColumn(col++, GetDetectionSourcePDG());
            analysisManager->FillNtupleDColumn(col++, GetDetectionSoilZ());
            analysisManager->FillNtupleDColumn(col++, GetDetectionSoilRho());
            analysisManager->AddNtupleRow();
        }
    }

    SetSoilSourceProperties(0.0, 0.0, 0.0, 0);
    SetDetectionProperties(0.0, 0, 0.0, 0.0);
}

void EventAction::RegisterPrimary(
    G4int trackID,
    const G4String& primaryName,
    G4int pdg
)
{
    primariesMap[trackID] = primaryName;
    pdgsMap[trackID] = pdg;
}

G4String EventAction::GetPrimaryName(const G4Track& track) const
{
    auto primary = primariesMap.find(track.GetParentID());
    if (primary != primariesMap.end()) {
        return primary->second;
    }

    return "unknown";
}

G4int EventAction::GetPrimaryPDG(const G4Track& track) const
{
    auto primary = pdgsMap.find(track.GetParentID());
    if (primary != pdgsMap.end()) {
        return primary->second;
    }

    return -1;
}

void EventAction::SetSoilSourceProperties(
    G4double depth,
    G4double rho,
    G4double energy,
    G4int pdg
)
{
    soilSourceZ = depth;
    soilSourceRho = rho;
    soilSourceEnergy = energy;
    soilSourcePDG = pdg;
}

void EventAction::SetDetectionProperties(
    G4double energyInSoil,
    G4int sourcePDG,
    G4double zInSoil,
    G4double rhoInSoil
)
{
    detectionEnergyInSoil = energyInSoil;
    detectionSourcePDG = sourcePDG;
    detectionZInSoil = zInSoil;
    detectionRhoInSoil = rhoInSoil;
}

G4double EventAction::GetSoilSourceZ() const
{
    return soilSourceZ;
}

G4double EventAction::GetSoilSourceRho() const
{
    return soilSourceRho;
}

G4double EventAction::GetSoilSourceEnergy() const
{
    return soilSourceEnergy;
}

G4int EventAction::GetSoilSourcePDG() const
{
    return soilSourcePDG;
}

G4double EventAction::GetDetectionStartingEnergyInSoil() const
{
    return detectionEnergyInSoil;
}

G4int EventAction::GetDetectionSourcePDG() const
{
    return detectionSourcePDG;
}

G4double EventAction::GetDetectionSoilZ() const
{
    return detectionZInSoil;
}

G4double EventAction::GetDetectionSoilRho() const
{
    return detectionRhoInSoil;
}