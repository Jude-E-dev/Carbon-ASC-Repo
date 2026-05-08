#include "EventAction.hh"

#include <G4AnalysisManager.hh>
#include <G4Event.hh>
#include <G4RunManager.hh>
#include <G4Track.hh>
#include <G4Types.hh>
#include <G4ios.hh>
#include <exception>
#include <variant>

#include "RunAction.hh"

EventAction::EventAction(RunAction* runAction, GCConfig config, int updatePeriod) :
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
        G4cout << "\n"
               << ev->GetEventID() / 1000 << "k events completed."
               << std::flush;
    }

    if (std::holds_alternative<ReplayEmissions>(config)) {
        auto analysisManager = G4AnalysisManager::Instance();
        int i = 0;
        if (GetPulseEnergy() > 0.001) {
            analysisManager->FillNtupleDColumn(1, i++, GetPulseEnergy());
            analysisManager->FillNtupleDColumn(1, i++, GetSourceDepth());
            analysisManager->FillNtupleDColumn(1, i++, GetSourceRadius());
            analysisManager->FillNtupleDColumn(1, i++, GetSourceEnergy());
            analysisManager->FillNtupleIColumn(1, i++, GetSourcePDG());
            analysisManager->AddNtupleRow(1);
        }
    }
    
    SetSourceProperties(0.0, 0.0, 0.0, 0);
    pulseEnergy = 0.0;
}

void EventAction::RegisterPrimary(int trackID, std::string primaryName, int pdg) {
    primariesMap[trackID] = primaryName;
    pdgsMap[trackID] = pdg;
}

std::string EventAction::GetPrimaryName(const G4Track* track) {
    std::string primary;
    try
    {
        primary = primariesMap.at(track->GetParentID());
    }
    catch (const std::exception& e)
    {
        primary = e.what();
    }
    return primary;
}

int EventAction::GetPrimaryPDG(const G4Track* track) {
    int primary;
    try
    {
        primary = pdgsMap.at(track->GetParentID());
    }
    catch (const std::exception&)
    {
        primary = -1;
    }
    return primary;
}

void EventAction::SetSourceProperties(double depth, double radius, double energy, int pdg) {
    sourceDepth = depth;
    sourceRadius = radius;
    sourceEnergy = energy;
    sourcePDG = pdg;
}

double EventAction::GetSourceDepth() {
    return sourceDepth;
}

double EventAction::GetSourceRadius() {
    return sourceRadius;
}

double EventAction::GetSourceEnergy() {
    return sourceEnergy;
}

int EventAction::GetSourcePDG() {
    return sourcePDG;
}

void EventAction::DepositEnergy(double eDep) {
    pulseEnergy += eDep;
}

double EventAction::GetPulseEnergy() {
    return pulseEnergy;
}