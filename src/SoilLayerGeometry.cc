#include "SoilLayerGeometry.hh"

#include <G4SystemOfUnits.hh>
#include <G4Types.hh>
#include <cmath>

std::vector<SoilLayerGeometry> ComputeSoilLayers(const SoilConfig& soil)
{
    using namespace SoilComposition;

    std::vector<SoilLayerGeometry> layers;
    layers.reserve(soil.soilCarbonSubdivisions);

    const G4double carbonDecayRate = CARBON_DECAY_RATE_PER_CM / cm;

    G4double zTop = 0.0;
    G4bool reachedBottom = false;
    for (std::size_t i = 0; i < soil.soilCarbonSubdivisions; i++) {
        G4double zBottom =
            std::log(1.0 - (G4double)(i + 1) / soil.soilCarbonSubdivisions)
            / carbonDecayRate;

        if (zBottom < -soil.soilDepth) {
            zBottom = -soil.soilDepth;
            reachedBottom = true;
        }
        if (i == soil.soilCarbonSubdivisions - 1) {
            zBottom = -soil.soilDepth;
        }

        const G4double carbonVolumeFraction =
            soil.soilCarbonPercentage * std::exp(carbonDecayRate * zBottom);
        const G4double mineralVolumeFraction = 1.0 - AIR_VOLUME_FRACTION
            - WATER_VOLUME_FRACTION - carbonVolumeFraction;

        layers.push_back(
            {zTop, zBottom, carbonVolumeFraction, mineralVolumeFraction}
        );

        zTop = zBottom;
        if (reachedBottom) {
            break;
        }
    }

    return layers;
}
