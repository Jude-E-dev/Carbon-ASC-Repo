#pragma once

#include <G4UserEventAction.hh>
#include <G4UserRunAction.hh>
#include <G4UserStackingAction.hh>
#include <G4UserSteppingAction.hh>
#include <G4UserTrackingAction.hh>
#include <G4VUserActionInitialization.hh>

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "v4/SoilModel.hh"
#include "v4/V4Generators.hh"
#include "v4/V4Options.hh"
#include "v4/V4Output.hh"
#include "v4/V4Records.hh"

class G4Track;

/**
 * @brief Where a gamma came from, captured on the step that created it and
 * consumed when the gamma is stacked.
 */
struct V4Origin
{
    std::int16_t proc = v4::PROC_OTHER;
    std::uint8_t targetZ = 0;
    std::uint8_t targetA = 0;
    std::int32_t parentPDG = 0;
    float parentE_MeV = 0.0F;
};

/** @brief State shared by the actions of one run. */
struct V4Shared
{
    V4Shared(const V4Options& o, const SoilModel& s) : options {o}, soil {s}, output {o} {}
    const V4Options& options;
    const SoilModel& soil;
    V4Output output;
    V4EventState event;
    std::unordered_map<const G4Track*, V4Origin> pending;
    std::vector<v4::TagCrossing> tagBuffer;
    std::uint64_t eventsDone = 0;
    int decayCounter = 0; // norm-library: radioactive decays so far this event
};

class V4RunAction: public G4UserRunAction
{
  public:
    explicit V4RunAction(V4Shared* shared) : shared {shared} {}
    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;

  private:
    V4Shared* shared;
    std::chrono::steady_clock::time_point start;
};

class V4EventAction: public G4UserEventAction
{
  public:
    explicit V4EventAction(V4Shared* shared) : shared {shared} {}
    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

  private:
    V4Shared* shared;
};

/**
 * @brief Records the origin of gamma secondaries (production, library) and
 * tallies neutron track length (production); in analog mode records
 * particles entering the tag volume.
 */
class V4SteppingAction: public G4UserSteppingAction
{
  public:
    explicit V4SteppingAction(V4Shared* shared) : shared {shared} {}
    void UserSteppingAction(const G4Step* step) override;

  private:
    V4Shared* shared;
};

/** @brief Propagates V4TrackInfo (primary PDG, origin) to secondaries. */
class V4TrackingAction: public G4UserTrackingAction
{
  public:
    explicit V4TrackingAction(V4Shared* shared) : shared {shared} {}
    void PreUserTrackingAction(const G4Track* track) override;
    void PostUserTrackingAction(const G4Track* track) override;

  private:
    V4Shared* shared;
};

/**
 * @brief Production and library modes: every gamma born in the soil above
 * the energy threshold is written out when it is stacked, then killed, so
 * none of its interactions are simulated here (transport does that).
 */
class V4StackingAction: public G4UserStackingAction
{
  public:
    explicit V4StackingAction(V4Shared* shared) : shared {shared} {}
    G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override;

  private:
    V4Shared* shared;
};

class V4ActionInitialization: public G4VUserActionInitialization
{
  public:
    explicit V4ActionInitialization(V4Shared* shared) : shared {shared} {}
    void Build() const override;

  private:
    V4Shared* shared;
};
