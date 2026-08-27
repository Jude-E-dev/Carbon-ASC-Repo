#pragma once

#include <G4Types.hh>
#include <vector>

#include "Configs.hh"

/**
 * @brief The z-extent and volume fractions of a single soil layer, as placed
 * by DetectorConstruction.
 *
 */
struct SoilLayerGeometry
{
    G4double zTop;
    G4double zBottom;
    G4double carbonVolumeFraction;
    G4double mineralVolumeFraction;
};

namespace SoilComposition
{
// Soil parameters, from McTaggart. Must match DetectorConstruction.cc.
inline constexpr G4double AIR_VOLUME_FRACTION = 0.25;
inline constexpr G4double WATER_VOLUME_FRACTION = 0.25;
inline constexpr G4double CARBON_DECAY_RATE_PER_CM = 0.02349;
} // namespace SoilComposition

/**
 * @brief Computes the per-layer depth boundaries and volume fractions used
 * to place soil volumes. Shared between DetectorConstruction (which builds
 * the materials/geometry) and NORMPrimaryGeneratorAction (which needs the
 * same per-layer mineral mass to weight isotope sampling) so the two never
 * drift out of sync.
 *
 * @param soil Configuration information about the soil
 */
std::vector<SoilLayerGeometry> ComputeSoilLayers(const SoilConfig& soil);
