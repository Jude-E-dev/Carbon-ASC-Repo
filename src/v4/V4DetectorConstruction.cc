#include "v4/V4DetectorConstruction.hh"

#include <G4Box.hh>
#include <G4Element.hh>
#include <G4Material.hh>
#include <G4NistManager.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4Tubs.hh>
#include <G4ios.hh>

#include <algorithm>
#include <map>
#include <stdexcept>

#include "v4/V4SensitiveDetector.hh"

V4DetectorConstruction::V4DetectorConstruction(
    const SoilModel& soil,
    const V4Options& options
)
    : soil {soil}
    , options {options}
{
}

G4VPhysicalVolume* V4DetectorConstruction::Construct()
{
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* germanium = nist->FindOrBuildMaterial("G4_Ge");

    /* ------------------------------- Phases -------------------------------- */

    // Hydrogen bound in liquid water can carry the ENDF thermal scattering law
    // (G4ThermalNeutrons maps the element name TS_H_of_Water to h_water).
    G4Element* tsHydrogen = nullptr;
    if (options.thermalScattering) {
        tsHydrogen = new G4Element("TS_H_of_Water", "H", 1.0, 1.0079 * g / mole);
    }

    std::map<std::string, G4Material*> phaseMaterials;
    for (const auto& phase : soil.Phases()) {
        auto* material = new G4Material(
            "phase_" + phase.name,
            phase.density_g_cm3 * g / cm3,
            static_cast<G4int>(phase.elements.size())
        );
        for (const auto& [symbol, fraction] : phase.elements) {
            G4Element* element = nullptr;
            if (tsHydrogen != nullptr && phase.name == "water" && symbol == "H") {
                element = tsHydrogen;
            }
            else {
                element = nist->FindOrBuildElement(symbol);
            }
            if (element == nullptr) {
                throw std::runtime_error("unknown element symbol " + symbol);
            }
            material->AddElement(element, fraction);
        }
        phaseMaterials[phase.name] = material;
    }

    /* -------------------------------- World -------------------------------- */

    const G4double halfWidth = options.worldHalfWidth_m * m;
    const G4double soilDepth = soil.Depth_cm() * cm;
    const G4double airHeight = options.airHeight_m * m;
    const G4double worldHalfZ = std::max(airHeight, soilDepth) + 1.0 * cm;

    auto* worldSolid = new G4Box("WorldSolid", halfWidth, halfWidth, worldHalfZ);
    auto* worldLogical = new G4LogicalVolume(worldSolid, vacuum, "WorldLogical");
    auto* worldPhysical = new G4PVPlacement(
        nullptr, {}, worldLogical, "WorldPhysical", nullptr, false, 0, true
    );

    auto* airSolid = new G4Box("AirSolid", halfWidth, halfWidth, airHeight / 2);
    auto* airLogical = new G4LogicalVolume(airSolid, air, "AirLogical");
    new G4PVPlacement(
        nullptr,
        G4ThreeVector(0, 0, airHeight / 2),
        airLogical,
        "AirPhysical",
        worldLogical,
        false,
        0,
        true
    );

    /* -------------------------------- Soil --------------------------------- */

    int index = 0;
    for (const auto& layer : soil.Layers()) {
        std::vector<std::pair<G4Material*, G4double>> parts;
        for (const auto& [phaseName, fraction] : layer.phases) {
            if (fraction > 0.0) {
                parts.emplace_back(phaseMaterials.at(phaseName), fraction);
            }
        }
        const std::string suffix = std::to_string(index);
        auto* material = new G4Material(
            "soil_layer_" + suffix,
            layer.density_g_cm3 * g / cm3,
            static_cast<G4int>(parts.size())
        );
        for (const auto& [phaseMaterial, fraction] : parts) {
            material->AddMaterial(phaseMaterial, fraction);
        }

        const G4double zTop = layer.zTop_cm * cm;
        const G4double zBottom = layer.zBottom_cm * cm;
        auto* solid = new G4Box(
            "SoilSolid_" + suffix, halfWidth, halfWidth, (zTop - zBottom) / 2
        );
        auto* logical = new G4LogicalVolume(solid, material, "SoilLogical_" + suffix);
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(0, 0, (zTop + zBottom) / 2),
            logical,
            "SoilPhysical_" + suffix,
            worldLogical,
            false,
            index,
            true
        );
        G4cout << "soil layer " << index << ": z " << layer.zTop_cm << " .. "
               << layer.zBottom_cm << " cm, rho " << layer.density_g_cm3
               << " g/cm3" << G4endl;
        index++;
    }

    /* ------------------------------ Detector ------------------------------- */

    if (options.mode == V4Options::Mode::Transport
        || options.mode == V4Options::Mode::Analog)
    {
        auto* detectorSolid = new G4Tubs(
            "SolidDetector",
            0.0,
            DETECTOR_RADIUS_CM * cm,
            DETECTOR_HEIGHT_CM * cm / 2,
            0.0,
            CLHEP::twopi
        );
        logicalDetector =
            new G4LogicalVolume(detectorSolid, germanium, "LogicalDetector");
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(0, 0, DETECTOR_ELEVATION_CM * cm - airHeight / 2),
            logicalDetector,
            "PhysicalDetector",
            airLogical,
            false,
            0,
            true
        );
    }

    if (options.mode == V4Options::Mode::Analog) {
        auto* tagSolid = new G4Box(
            "TagSolid", TAG_HALF_WIDTH_CM * cm, TAG_HALF_WIDTH_CM * cm, 0.5 * cm
        );
        auto* tagLogical = new G4LogicalVolume(tagSolid, air, "TagLogical");
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(0, 0, TAG_ELEVATION_CM * cm - airHeight / 2),
            tagLogical,
            "TagVolume",
            airLogical,
            false,
            0,
            true
        );
    }

    return worldPhysical;
}

void V4DetectorConstruction::ConstructSDandField()
{
    if (logicalDetector != nullptr) {
        auto* sd = new V4SensitiveDetector("GeSD");
        G4SDManager::GetSDMpointer()->AddNewDetector(sd);
        logicalDetector->SetSensitiveDetector(sd);
    }
}
