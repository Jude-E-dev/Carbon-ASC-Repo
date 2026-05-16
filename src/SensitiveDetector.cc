#include "SensitiveDetector.hh"

#include <G4AnalysisManager.hh>
#include <G4RunManager.hh>
#include <G4SDManager.hh>
#include <G4THitsCollection.hh>
#include <G4TrackStatus.hh>
#include <G4VSensitiveDetector.hh>
#include <G4ios.hh>

#include "EventAction.hh"

SensitiveDetector::SensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name)
{
    collectionName.insert("hits");
}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce)
{
    hitsCollection = new G4THitsCollection<DetectorHit>(
        SensitiveDetectorName,
        collectionName[0]
    );

    if (hcID < 0) {
        hcID = G4SDManager::GetSDMpointer()->GetCollectionID(hitsCollection);
    }

    hce->AddHitsCollection(hcID, hitsCollection);
}

G4bool
SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* /*unused*/)
{
    G4double eDep = step->GetTotalEnergyDeposit();

    if (eDep == 0.0) {
        return false;
    }

    auto* hit = new DetectorHit();
    hit->SetEdep(eDep);
    hitsCollection->insert(hit);
    return true;
}