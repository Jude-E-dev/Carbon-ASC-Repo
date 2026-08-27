#include "NORMPrimaryGeneratorAction.hh"

#include <G4Event.hh>
#include <G4IonTable.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleGun.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4Types.hh>
#include <stdexcept>

#include "SoilLayerGeometry.hh"

namespace
{
// Natural potassium mass fraction of the mineral component. Must match
// DetectorConstruction.cc's elK mass fraction (North Canberra XRF/ICP-MS
// median, 1.403%) -- these are the same atoms, just viewed from the
// source-term side instead of the material side.
constexpr G4double POTASSIUM_MASS_FRACTION = 0.01403;
// Bq per gram of natural potassium (UNSCEAR / IAEA reference value).
constexpr G4double K40_SPECIFIC_ACTIVITY_BQ_PER_G_K = 30.4;

// Trace U/Th concentrations of the mineral component, in parts-per-million
// by mass. These are NOT in the bulk material definition (too dilute to
// affect density), so they're independent, directly-editable knobs here.
//   EDIT THESE to change the simulated NORM background level.
constexpr G4double URANIUM_CONCENTRATION_PPM = 2.0; // typical soil: 1-3 ppm U
constexpr G4double THORIUM_CONCENTRATION_PPM = 10.0; // typical soil: 2-12 ppm Th

// Specific activity of the pure parent nuclide (Bq/g), from T_1/2 via
// A = ln(2) * N_A / (T_1/2 * atomic_mass).
constexpr G4double U238_SPECIFIC_ACTIVITY_BQ_PER_G = 12350.0;
constexpr G4double TH232_SPECIFIC_ACTIVITY_BQ_PER_G = 4060.0;

constexpr G4double PPM_TO_MASS_FRACTION = 1.0e-6;
constexpr G4double G_TO_KG = 1.0e-3; // g -> kg divisor helper (multiply by this)

// Must match DetectorConstruction.cc's mineralComponent density.
constexpr G4double MINERAL_DENSITY = 2.6 * g / cm3;
} // namespace

NORMPrimaryGeneratorAction::NORMPrimaryGeneratorAction(const SoilConfig& soil)
    : totalActivityBq {0.0}
    , soilWidth {soil.soilWidth}
    , gun {new G4ParticleGun()}
    , rng {std::random_device {}()}
    , uniformDist(0.0, 1.0)
{
    // specificActivity is Bq per kg of mineral component.
    isotopes = {
        {19,
         40,
         POTASSIUM_MASS_FRACTION * K40_SPECIFIC_ACTIVITY_BQ_PER_G_K / G_TO_KG},
        {92,
         238,
         URANIUM_CONCENTRATION_PPM * PPM_TO_MASS_FRACTION
             * U238_SPECIFIC_ACTIVITY_BQ_PER_G / G_TO_KG},
        {90,
         232,
         THORIUM_CONCENTRATION_PPM * PPM_TO_MASS_FRACTION
             * TH232_SPECIFIC_ACTIVITY_BQ_PER_G / G_TO_KG},
    };

    BuildSourceCDF(soil);
}

void NORMPrimaryGeneratorAction::BuildSourceCDF(const SoilConfig& soil)
{
    const auto layers = ComputeSoilLayers(soil);

    for (std::size_t layerIdx = 0; layerIdx < layers.size(); layerIdx++) {
        const auto& layer = layers[layerIdx];
        const G4double layerVolume =
            soil.soilWidth * soil.soilWidth * (layer.zTop - layer.zBottom);
        const G4double mineralMassKg =
            layer.mineralVolumeFraction * layerVolume * MINERAL_DENSITY / kg;

        for (std::size_t isotopeIdx = 0; isotopeIdx < isotopes.size();
             isotopeIdx++)
        {
            const G4double activityBq =
                mineralMassKg * isotopes[isotopeIdx].specificActivity;

            if (activityBq <= 0.0) {
                continue;
            }

            sourceCDF.push_back(
                {layerIdx, isotopeIdx, layer.zTop, layer.zBottom, activityBq}
            );
            totalActivityBq += activityBq;
        }
    }

    if (sourceCDF.empty()) {
        throw std::runtime_error(
            "NORMPrimaryGeneratorAction: no soil layers with nonzero NORM "
            "activity -- check SoilConfig and isotope concentrations."
        );
    }
}

const NORMPrimaryGeneratorAction::LayerSource&
NORMPrimaryGeneratorAction::SampleSource()
{
    G4double target = uniformDist(rng) * totalActivityBq;
    for (const auto& source : sourceCDF) {
        target -= source.activityBq;
        if (target <= 0.0) {
            return source;
        }
    }
    return sourceCDF.back();
}

void NORMPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    const auto& source = SampleSource();
    auto& isotope = isotopes[source.isotopeIdx];

    if (isotope.definition == nullptr) {
        isotope.definition =
            G4IonTable::GetIonTable()->GetIon(isotope.Z, isotope.A, 0.0);
    }

    const G4double x = soilWidth * (uniformDist(rng) - 0.5);
    const G4double y = soilWidth * (uniformDist(rng) - 0.5);
    const G4double z =
        source.zTop + uniformDist(rng) * (source.zBottom - source.zTop);

    gun->SetParticleDefinition(isotope.definition);
    gun->SetParticleEnergy(0.0); // decay at rest
    gun->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
    gun->SetParticlePosition(G4ThreeVector(x, y, z));
    gun->SetParticleTime(0.0);

    gun->GeneratePrimaryVertex(event);
    eventsGenerated++;
}

G4double NORMPrimaryGeneratorAction::GetExposureTime() const
{
    return (G4double)eventsGenerated / totalActivityBq;
}
