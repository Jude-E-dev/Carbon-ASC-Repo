#pragma once

#include <G4VSensitiveDetector.hh>
#include <G4VUserTrackInformation.hh>

#include <array>
#include <cstdint>
#include <map>

/**
 * @brief Per-track bookkeeping: which cosmic primary (or NORM parent) a
 * track descends from, and its origin class (v4::TrackOrigin): direct from
 * the air shower, a descendant of a soil-born gamma, or of another
 * soil-born particle.
 */
class V4TrackInfo: public G4VUserTrackInformation
{
  public:
    V4TrackInfo(std::int32_t primaryPDG, int origin, int decayId = 0)
        : primaryPDG {primaryPDG}
        , origin {origin}
        , decayId {decayId}
    {
    }
    std::int32_t primaryPDG;
    int origin;
    /// norm-library: index of the radioactive decay (within the chain) this
    /// track descends from, so prompt coincidence groups can be rebuilt.
    int decayId;
};

/**
 * @brief Germanium sensitive detector. Sums the energy deposited in each
 * event, and also bins it into 100 ns time buckets split by track origin, so
 * analog mode can rebuild pulses and coincidences afterwards.
 */
class V4SensitiveDetector: public G4VSensitiveDetector
{
  public:
    explicit V4SensitiveDetector(const G4String& name);
    void Initialize(G4HCofThisEvent* hce) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;

    static constexpr double BUCKET_NS = 100.0;

    struct Bucket
    {
        std::array<double, 3> edepByOrigin {0.0, 0.0, 0.0}; // MeV
        double largest = 0.0;
        std::int32_t primaryPDG = 0;
    };

    G4double TotalEdep() const { return totalEdep; }
    const std::map<std::int64_t, Bucket>& Buckets() const { return buckets; }

  private:
    G4double totalEdep = 0.0;
    std::map<std::int64_t, Bucket> buckets;
};
