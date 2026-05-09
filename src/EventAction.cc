#include "EventAction.hh"

#include <G4AnalysisManager.hh>
#include <G4Event.hh>
#include <G4RunManager.hh>
#include <G4String.hh>
#include <G4Track.hh>
#include <G4Types.hh>
#include <G4ios.hh>
#include <exception>
#include <variant>

#include "Configs.hh"
#include "TransportPrimaryGeneratorAction.hh"

EventAction::EventAction(const GCConfig& config, G4int updatePeriod)
    : G4UserEventAction()
    , config {config}
    , updatePeriod {updatePeriod}
    , primariesMap {}
    , pdgsMap {}
{
}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event* ev)
{
    primariesMap.clear();
    pdgsMap.clear();
}

void EventAction::EndOfEventAction(const G4Event* ev)
{
    G4int n = ev->GetEventID();
    if (n % updatePeriod == 0) {
        G4cout << ev->GetEventID() / 1000 << "k events completed." << G4endl;
    }

    if (std::holds_alternative<Detect>(config)) {
        auto analysisManager = G4AnalysisManager::Instance();
        G4int i = 0;
        if (GetDetectionPulseEnergy() > 0.001) {
            analysisManager->FillNtupleDColumn(i++, GetDetectionPulseEnergy());
            analysisManager->FillNtupleDColumn(
                i++,
                GetDetectionStartingEnergyInSoil()
            );
            analysisManager->FillNtupleIColumn(i++, GetDetectionSourcePDG());
            analysisManager->FillNtupleDColumn(i++, GetDetectionSoilZ());
            analysisManager->FillNtupleDColumn(i++, GetDetectionSoilRho());
            analysisManager->AddNtupleRow();
        }
    }

    SetSoilSourceProperties(0.0, 0.0, 0.0, 0);
    SetDetectionProperties(0.0, 0, 0.0, 0.0);
    detectionPulseEnergy = 0.0;
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

G4String EventAction::GetPrimaryName(const G4Track* track) const
{
    G4String primary;
    try {
        primary = primariesMap.at(track->GetParentID());
    }
    catch (const std::exception& e) {
        primary = e.what();
    }
    return primary;
}

G4int EventAction::GetPrimaryPDG(const G4Track* track) const
{
    G4int primary;
    try {
        primary = pdgsMap.at(track->GetParentID());
    }
    catch (const std::exception&) {
        primary = -1;
    }
    return primary;
}

void EventAction::SetSoilSourceProperties(
    G4double z,
    G4double rho,
    G4double energy,
    G4int pdg
)
{
    soilSourceZ = z;
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

void EventAction::DepositEnergy(G4double eDep)
{
    detectionPulseEnergy += eDep;
}

G4double EventAction::GetDetectionPulseEnergy() const
{
    return detectionPulseEnergy;
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