#include "EventAction.hh"

#include <G4AnalysisManager.hh>
#include <G4Event.hh>
#include <G4RunManager.hh>
#include <G4Track.hh>
#include <G4Types.hh>
#include <G4ios.hh>
#include <exception>
#include <variant>

#include "Configs.hh"
#include "RunAction.hh"
#include "TransportPrimaryGeneratorAction.hh"

EventAction::EventAction(
    RunAction* runAction,
    GCConfig config,
    int updatePeriod
) :
    G4UserEventAction(),
    config {config},
    updatePeriod {updatePeriod},
    primariesMap(),
    pdgsMap() {}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event* ev) {
    primariesMap.clear();
    pdgsMap.clear();
}

void EventAction::EndOfEventAction(const G4Event* ev) {
    G4int n = ev->GetEventID();
    if (n % updatePeriod == 0) {
        G4cout << ev->GetEventID() / 1000 << "k events completed." << G4endl;
    }

    if (std::holds_alternative<Detect>(config)) {
        auto analysisManager = G4AnalysisManager::Instance();
        int i = 0;
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
    int trackID,
    std::string primaryName,
    int pdg
) {
    primariesMap[trackID] = primaryName;
    pdgsMap[trackID] = pdg;
}

std::string EventAction::GetPrimaryName(const G4Track* track) const {
    std::string primary;
    try {
        primary = primariesMap.at(track->GetParentID());
    } catch (const std::exception& e) {
        primary = e.what();
    }
    return primary;
}

int EventAction::GetPrimaryPDG(const G4Track* track) const {
    int primary;
    try {
        primary = pdgsMap.at(track->GetParentID());
    } catch (const std::exception&) {
        primary = -1;
    }
    return primary;
}

void EventAction::SetSoilSourceProperties(
    double z,
    double rho,
    double energy,
    int pdg
) {
    soilSourceZ = z;
    soilSourceRho = rho;
    soilSourceEnergy = energy;
    soilSourcePDG = pdg;
}

void EventAction::SetDetectionProperties(
    double energyInSoil,
    int sourcePDG,
    double zInSoil,
    double rhoInSoil
) {
    detectionEnergyInSoil = energyInSoil;
    detectionSourcePDG = sourcePDG;
    detectionZInSoil = zInSoil;
    detectionRhoInSoil = rhoInSoil;
}

double EventAction::GetSoilSourceZ() const {
    return soilSourceZ;
}

double EventAction::GetSoilSourceRho() const {
    return soilSourceRho;
}

double EventAction::GetSoilSourceEnergy() const {
    return soilSourceEnergy;
}

int EventAction::GetSoilSourcePDG() const {
    return soilSourcePDG;
}

void EventAction::DepositEnergy(double eDep) {
    detectionPulseEnergy += eDep;
}

double EventAction::GetDetectionPulseEnergy() const {
    return detectionPulseEnergy;
}

double EventAction::GetDetectionStartingEnergyInSoil() const {
    return detectionEnergyInSoil;
}

int EventAction::GetDetectionSourcePDG() const {
    return detectionSourcePDG;
}

double EventAction::GetDetectionSoilZ() const {
    return detectionZInSoil;
}

double EventAction::GetDetectionSoilRho() const {
    return detectionRhoInSoil;
}