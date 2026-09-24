#include "v4/V4SensitiveDetector.hh"

#include <G4Step.hh>
#include <G4SystemOfUnits.hh>
#include <G4Track.hh>

#include <cmath>

V4SensitiveDetector::V4SensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name)
{
}

void V4SensitiveDetector::Initialize(G4HCofThisEvent* /*hce*/)
{
    totalEdep = 0.0;
    buckets.clear();
}

G4bool V4SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* /*history*/)
{
    const G4double edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.0) {
        return false;
    }
    totalEdep += edep;

    const double t_ns = step->GetPreStepPoint()->GetGlobalTime() / ns;
    const auto index = static_cast<std::int64_t>(std::floor(t_ns / BUCKET_NS));
    const auto* info =
        dynamic_cast<const V4TrackInfo*>(step->GetTrack()->GetUserInformation());
    const int origin = info != nullptr ? info->origin : 0;

    auto& bucket = buckets[index];
    bucket.edepByOrigin[origin] += edep / MeV;
    if (info != nullptr && edep / MeV > bucket.largest) {
        bucket.largest = edep / MeV;
        bucket.primaryPDG = info->primaryPDG;
    }
    return true;
}
