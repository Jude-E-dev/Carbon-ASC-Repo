#pragma once

#include <G4ParticleGun.hh>
#include <G4Types.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <cstddef>
#include <random>
#include <vector>

#include "Configs.hh"

class G4Event;
class G4ParticleDefinition;

/**
 * @brief Primary generator that injects NORM ions (K-40, U-238, Th-232) at
 * rest, distributed through the soil volume weighted by per-layer mineral
 * mass, letting G4RadioactiveDecay produce the decay-chain gammas.
 *
 */
class NORMPrimaryGeneratorAction: public G4VUserPrimaryGeneratorAction
{
  public:
    explicit NORMPrimaryGeneratorAction(const SoilConfig& soil);

    ~NORMPrimaryGeneratorAction() override = default;
    NORMPrimaryGeneratorAction(const NORMPrimaryGeneratorAction&) = delete;
    NORMPrimaryGeneratorAction(NORMPrimaryGeneratorAction&&) = delete;
    NORMPrimaryGeneratorAction&
    operator=(const NORMPrimaryGeneratorAction&) = delete;
    NORMPrimaryGeneratorAction& operator=(NORMPrimaryGeneratorAction&&) = delete;

    void GeneratePrimaries(G4Event* event) override;

    /**
     * @brief Get the amount of time that would need to pass to achieve this
     * exposure, in seconds.
     */
    G4double GetExposureTime() const;

  private:
    struct Isotope
    {
        G4int Z;
        G4int A;
        G4double specificActivity; // Bq/kg mineral
        // Fetched once in the constructor (before /run/initialize finishes
        // building physics tables) rather than lazily per-event -- ions
        // instantiated for the first time *during* the event loop don't get
        // RadioactiveDecay properly wired up. See G4's rdecay01/02 examples.
        G4ParticleDefinition* definition = nullptr;
    };

    struct LayerSource
    {
        std::size_t layerIdx;
        std::size_t isotopeIdx;
        G4double zTop;
        G4double zBottom;
        G4double activityBq;
    };

    void BuildSourceCDF(const SoilConfig& soil);
    const LayerSource& SampleSource();

    std::vector<Isotope> isotopes;
    std::vector<LayerSource> sourceCDF;
    G4double totalActivityBq;
    G4double soilWidth;
    unsigned long eventsGenerated = 0;

    G4ParticleGun* gun;
    std::mt19937 rng;
    std::uniform_real_distribution<G4double> uniformDist;
};
