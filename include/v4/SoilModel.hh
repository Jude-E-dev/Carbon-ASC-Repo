#pragma once

#include <G4Types.hh>

#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief A soil column read at run time from a `.soil` text file (written by
 * the Python soil builder, Geochemical Survey/Soil_Types_JSON_v2/build_soil.py).
 *
 * The file describes a set of phases (mineral, water, organic matter, air,
 * each with element mass fractions) and a stack of layers, each with a bulk
 * density and the mass fraction of every phase. All soil chemistry is derived
 * in Python; this class only reads and checks it.
 *
 *   gc-soil 1
 *   name <id>
 *   phase <name> <density g/cm3> <n> <El> <mass frac> ...
 *   layer <z top cm> <z bottom cm> <density g/cm3> <n> <phase> <mass frac> ...
 *   end
 *
 * z is measured upwards from the soil surface, so layers have z <= 0.
 */
struct SoilPhase
{
    std::string name;
    G4double density_g_cm3 = 0.0;
    std::vector<std::pair<std::string, G4double>> elements; // symbol, mass frac
};

struct SoilLayer
{
    G4double zTop_cm = 0.0;
    G4double zBottom_cm = 0.0;
    G4double density_g_cm3 = 0.0;
    std::vector<std::pair<std::string, G4double>> phases; // phase, mass frac

    G4double PhaseMassFraction(const std::string& phase) const;
};

class SoilModel
{
  public:
    static SoilModel Load(const std::string& path);

    const std::string& Name() const { return name; }
    const std::vector<SoilPhase>& Phases() const { return phases; }
    const std::vector<SoilLayer>& Layers() const { return layers; }
    const SoilPhase* FindPhase(const std::string& phaseName) const;

    /// Depth of the bottom of the column, as a positive number of cm.
    G4double Depth_cm() const;

    /// Mass depth (g/cm2) from the surface down to height z (cm, z <= 0).
    G4double MassDepth(G4double z_cm) const;

    /// Mass per unit area (g/cm2) of `phase` between the surface and depth
    /// `maxDepth_cm` (positive).
    G4double PhaseAreal(const std::string& phase, G4double maxDepth_cm) const;

    /// Samples a height z (cm) distributed as the mass of `phase` per unit
    /// depth, restricted to the top `maxDepth_cm`. u1, u2 are uniform(0,1).
    G4double SamplePhaseDepth(
        const std::string& phase,
        G4double maxDepth_cm,
        G4double u1,
        G4double u2
    ) const;

    std::string sourcePath;

  private:
    std::string name;
    std::vector<SoilPhase> phases;
    std::vector<SoilLayer> layers;
};
