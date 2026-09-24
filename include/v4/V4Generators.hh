#pragma once

#include <G4ThreeVector.hh>
#include <G4Types.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include <CRYGenerator.h>
#include <CRYParticle.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "v4/SoilModel.hh"
#include "v4/V4Options.hh"
#include "v4/V4Records.hh"

class G4ParticleGun;
class G4ParticleDefinition;

/**
 * @brief State the transport generator hands to the event action.
 */
struct V4EventState
{
    double weight = 0.0;
    int originClass = 0;
    int batch = 0;
    double zSource_cm = 0.0;
};

/** @brief Base with an exposure/area report used by the run action. */
class V4Generator: public G4VUserPrimaryGeneratorAction
{
  public:
    virtual double Exposure_s() const { return 0.0; }
    virtual double SourceArea_cm2() const { return 0.0; }
    virtual std::string ExtraJson() const { return "{}"; }
    /// Events the run should process (0 = use --events).
    virtual std::uint64_t PlannedEvents() const { return 0; }
};

/**
 * @brief CRY cosmic-ray showers on a square box of side subboxLength (read
 * from the setup file), at height `zOffset` above the ground.
 */
class V4CRYGenerator: public V4Generator
{
  public:
    V4CRYGenerator(const std::string& setupPath, double zOffset_cm);
    void GeneratePrimaries(G4Event* event) override;
    double Exposure_s() const override;
    double SourceArea_cm2() const override { return boxLength_cm * boxLength_cm; }
    std::string ExtraJson() const override;

  private:
    std::unique_ptr<CRYGenerator> generator;
    std::vector<CRYParticle*> particles;
    G4ParticleGun* gun;
    double zOffset_cm;
    double boxLength_cm = 0.0;
    std::string setupText;
};

/**
 * @brief Test beam: a particle of fixed energy going straight down onto a
 * random point of a square of side gunBox at the ground surface.
 */
class V4GunGenerator: public V4Generator
{
  public:
    explicit V4GunGenerator(const V4Options& options);
    void GeneratePrimaries(G4Event* event) override;
    double SourceArea_cm2() const override { return box_cm * box_cm; }

  private:
    G4ParticleGun* gun;
    double box_cm;
};

/**
 * @brief NORM decay library: a K-40, U-238 or Th-232 nucleus at rest inside
 * the soil; G4RadioactiveDecay runs the whole chain (secular equilibrium).
 */
class V4NormLibraryGenerator: public V4Generator
{
  public:
    explicit V4NormLibraryGenerator(const V4Options& options);
    void GeneratePrimaries(G4Event* event) override;

  private:
    G4ParticleGun* gun;
    int Z;
    int A;
    G4ParticleDefinition* ion = nullptr;
};

/**
 * @brief Transport source. Replays production records (GC4PROD, one gamma per
 * event, `replays` times each) or samples NORM library groups (GC4NLIB, all
 * gammas of one prompt group per event), placing each at a lateral position
 * drawn by importance sampling around the detector, and hands the weight
 *   w = roulette / (replays * q(x))        [cm^2]
 * to the event action. Summed weights divided by (exposure * source area)
 * give counts per second for an infinite, uniformly irradiated field; for
 * NORM see ExtraJson().
 */
class V4TransportGenerator: public V4Generator
{
  public:
    V4TransportGenerator(const V4Options& options, const SoilModel& soil,
                         V4EventState* state);
    void GeneratePrimaries(G4Event* event) override;
    double Exposure_s() const override { return inputHeader.exposure_s; }
    double SourceArea_cm2() const override { return inputHeader.sourceArea_cm2; }
    std::string ExtraJson() const override;
    std::uint64_t PlannedEvents() const override;

  private:
    struct Placement
    {
        double x_cm;
        double y_cm;
        double invQ_cm2; // 1/q(x)
    };
    Placement SamplePosition(const G4ThreeVector& direction, double z_cm);
    double ShadowDensity(double x, double y, const G4ThreeVector& d, double z) const;
    double BroadDensity(double x, double y) const;
    double RouletteWeight(double E_MeV, double z_cm);
    bool NextProductionRecord(v4::ProductionRecord& out);
    void LoadLibrary();

    const V4Options& options;
    const SoilModel& soil;
    V4EventState* state;
    G4ParticleGun* gun;
    std::ifstream input;
    v4::FileHeader inputHeader {};
    bool isLibrary = false;

    // production replay
    v4::ProductionRecord current {};
    int replayIndex = 0;
    bool haveCurrent = false;
    std::uint64_t recordsRead = 0;

    // NORM library
    std::vector<std::vector<v4::ProductionRecord>> groups;
    std::uint64_t libraryChains = 0;
    double normArealMineral_g_cm2 = 0.0;
    std::uint64_t samplesDone = 0;

    std::uint64_t rouletteKilled = 0;
    std::uint64_t shadowDraws = 0;
};
