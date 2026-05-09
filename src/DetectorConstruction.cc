#include "DetectorConstruction.hh"

#include <CLHEP/Units/SystemOfUnits.h>

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4NistManager.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4Types.hh>
#include <G4VisAttributes.hh>
#include <G4ios.hh>
#include <cmath>
#include <variant>

#include "Configs.hh"
#include "SensitiveDetector.hh"

DetectorConstruction::DetectorConstruction(
    const SoilConfig& soil,
    const GCConfig& gcConfig
)
    : G4VUserDetectorConstruction {}
    , soilCarbonPercentage {soil.soilCarbonPercentage}
    , soilCarbonSubdivisions {soil.soilCarbonSubdivisions}
    , soilDepth {soil.soilDepth}
    , soilWidth {soil.soilWidth}
    , soilLogicalLayers(soil.soilCarbonSubdivisions, nullptr)
    , config {gcConfig}
    , logicalDetector {nullptr}
{
}

DetectorConstruction::~DetectorConstruction() {}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto* nist = G4NistManager::Instance();

    /* -------------------------------- Materials ------------------------------- */

    auto* elO = new G4Element("Oxygen", "O", 8., 16.00 * g / mole);
    auto* elSi = new G4Element("Silicon", "Si", 14., 28.09 * g / mole);
    auto* elAl = new G4Element("Aluminum", "Al", 13., 26.98 * g / mole);
    auto* elFe = new G4Element("Iron", "Fe", 26., 55.85 * g / mole);
    auto* elCa = new G4Element("Calcium", "Ca", 20., 69.72 * g / mole);
    auto* elNa = new G4Element("Sodium", "Na", 11., 22.99 * g / mole);
    auto* elK = new G4Element("Potassium", "K", 19., 39.1 * g / mole);
    auto* elMg = new G4Element("Magnesium", "Mg", 12., 24.31 * g / mole);
    auto* elTi = new G4Element("Titanium", "Ti", 22., 47.87 * g / mole);
    auto* elMn = new G4Element("Manganese", "Mn", 25., 54.94 * g / mole);
    auto* elC = new G4Element("Carbon", "C", 6., 12.01 * g / mole);
    auto* elH = new G4Element("Hydrogen", "H", 1., 1.01 * g / mole);
    auto* elN = new G4Element("Nitrogen", "N", 7., 14.01 * g / mole);
    auto* elP = new G4Element("Phosphorus", "P", 15., 30.97 * g / mole);
    auto* elS = new G4Element("Sulfur", "S", 16., 32.07 * g / mole);

    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    auto* germanium = nist->FindOrBuildMaterial("G4_Ge");
    auto* soilOrganicCarbon =
        new G4Material("soilOrganicCarbon", 1.2 * g / cm3, 6);
    soilOrganicCarbon->AddElement(elC, 0.58);
    soilOrganicCarbon->AddElement(elO, 0.22);
    soilOrganicCarbon->AddElement(elH, 0.12);
    soilOrganicCarbon->AddElement(elN, 0.06);
    soilOrganicCarbon->AddElement(elP, 0.01);
    soilOrganicCarbon->AddElement(elS, 0.01);

    // From McTaggart
    auto* mineralComponent =
        new G4Material("mineralComponent", 2.6 * g / cm3, 10);
    mineralComponent->AddElement(elO, 0.440);
    mineralComponent->AddElement(elSi, 0.280);
    mineralComponent->AddElement(elAl, 0.080);
    mineralComponent->AddElement(elFe, 0.050);
    mineralComponent->AddElement(elCa, 0.035);
    mineralComponent->AddElement(elNa, 0.030);
    mineralComponent->AddElement(elK, 0.025);
    mineralComponent->AddElement(elMg, 0.020);
    mineralComponent->AddElement(elTi, 0.005);
    mineralComponent->AddElement(elMn, 0.001);
    // Remaining 3.4% is 'other'

    /* ---------------------------------- World --------------------------------- */

    auto* solidWorld = new G4Box(
        "WorldSolid",
        0.5 * soilWidth,
        0.5 * soilWidth,
        soilDepth // twice as long as soil depth
    );

    auto* logicWorld = new G4LogicalVolume(solidWorld, air, "WorldLogical");

    auto* visWorld = new G4VisAttributes(G4Colour(0.3, 0.4, 0.9, 1.0));
    visWorld->SetForceWireframe(true);
    logicWorld->SetVisAttributes(visWorld);

    auto* physWorld = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logicWorld,
        "WorldPhysical",
        nullptr,
        false,
        0,
        true
    );

    /* -------------------------- Germanium Detector -------------------------- */

    if (!std::holds_alternative<CharacteriseSoilGammas>(config)) {
        auto* detectorCylinder = new G4Tubs(
            "SolidDetector",
            0.0,
            8.3 * cm,
            16.8 * cm,
            0.0,
            CLHEP::twopi
        );

        logicalDetector =
            new G4LogicalVolume(detectorCylinder, germanium, "LogicalDetector");

        new G4PVPlacement(
            nullptr,
            G4ThreeVector(0.0, 0.0, 20 * cm + 16.8 * cm / 2.0),
            "PhysicalDetector",
            logicalDetector,
            physWorld,
            false,
            0,
            true
        );
    }

    if (std::holds_alternative<Detect>(config))
        return physWorld;

    /* ------------------------------ Soil Volumes ------------------------------ */
    /* -------------- https://www.desmos.com/calculator/tcl1fcxpzj -------------- */

    const G4double airDensity = air->GetDensity() / (g / cm3);
    const G4double waterDensity = water->GetDensity() / (g / cm3);
    const G4double carbonDensity = soilOrganicCarbon->GetDensity() / (g / cm3);
    const G4double mineralDensity = mineralComponent->GetDensity() / (g / cm3);

    // Soil parameters, from McTaggart
    const G4double airVolumeFraction = 0.25;
    const G4double waterVolumeFraction = 0.25;
    const G4double k = 0.02349 / cm;

    // Construct layers
    G4double zTop = 0.0;
    G4double reachedBottom = false;
    for (size_t i = 0; i < soilCarbonSubdivisions; i++) {
        G4double zBottom =
            std::log(1.0 - (G4double)(i + 1) / soilCarbonSubdivisions) / k;

        if (zBottom < -soilDepth) {
            zBottom = -soilDepth;
            reachedBottom = true;
        }
        if (i == soilCarbonSubdivisions - 1) {
            zBottom = -soilDepth;
        }

        const G4double carbonVolumeFraction =
            soilCarbonPercentage * std::exp(k * zBottom);
        const G4double mineralVolumeFraction = 1.0 - airVolumeFraction
            - waterVolumeFraction - carbonVolumeFraction;

        const G4double totalMass = airVolumeFraction * airDensity
            + waterVolumeFraction * waterDensity
            + carbonVolumeFraction * carbonDensity
            + mineralVolumeFraction * mineralDensity;

        const G4double density = totalMass
            / (airVolumeFraction + waterVolumeFraction + carbonVolumeFraction
               + mineralVolumeFraction);

        const G4String soilName = "SoilMixtureLayer_" + std::to_string(i);
        auto* soilMixture = new G4Material(soilName, density * g / cm3, 4);

        const G4double airMassFraction =
            airVolumeFraction * airDensity / density;
        const G4double waterMassFraction =
            waterVolumeFraction * waterDensity / density;
        const G4double soilMassFraction =
            carbonVolumeFraction * carbonDensity / density;
        const G4double mineralMassFraction =
            1.0 - airMassFraction - waterMassFraction - soilMassFraction;

        soilMixture->AddMaterial(air, airMassFraction);
        soilMixture->AddMaterial(water, waterMassFraction);
        soilMixture->AddMaterial(soilOrganicCarbon, soilMassFraction);
        soilMixture->AddMaterial(mineralComponent, mineralMassFraction);

        G4cout << soilName << ": {\n\t"
               << "pos: {z top: " << zTop << ", z bot: " << zBottom << "},\n\t"
               << "air: { dens: " << airDensity
               << ", vol frac: " << airVolumeFraction
               << ", mass frac: " << airMassFraction << " },\n\t"
               << "water: { dens: " << waterDensity
               << ", vol frac: " << waterVolumeFraction
               << ", mass frac: " << waterMassFraction << " },\n\t"
               << "carbon: { dens: " << carbonDensity
               << ", vol frac: " << carbonVolumeFraction
               << ", mass frac: " << soilMassFraction << " },\n\t"
               << "mineral: { dens: " << mineralDensity
               << ", vol frac: " << mineralVolumeFraction
               << ", mass frac: " << mineralMassFraction << " },\n\t"
               << "total: { dens: " << density << ", vol frac: "
               << airVolumeFraction + waterVolumeFraction + carbonVolumeFraction
                + mineralVolumeFraction
               << ", mass frac: "
               << airMassFraction + waterMassFraction + soilMassFraction
                + (mineralVolumeFraction * mineralDensity / density)
               << " }\n"
               << "}" << G4endl;

        const G4String nameSolid = "SolidLayer_" + std::to_string(i);
        const G4String nameLogical = "LogicalLayer_" + std::to_string(i);
        const G4String namePhysical = "PhysicalLayer_" + std::to_string(i);

        auto* solidLayer = new G4Box(
            nameSolid,
            0.5 * soilWidth,
            0.5 * soilWidth,
            0.5 * (zTop - zBottom)
        );

        soilLogicalLayers[i] =
            new G4LogicalVolume(solidLayer, soilMixture, nameLogical);

        const G4double t = 0.8 + 0.5 * (carbonVolumeFraction / 0.15);
        auto* visSoil =
            new G4VisAttributes(G4Colour(0.74 * t, 0.55 * t, 0.45 * t, 1.0));
        visSoil->SetForceWireframe(true);
        soilLogicalLayers[i]->SetVisAttributes(visSoil);

        new G4PVPlacement(
            nullptr,
            G4ThreeVector(0, 0, 0.5 * (zTop + zBottom)),
            soilLogicalLayers[i],
            namePhysical,
            logicWorld,
            false,
            i,
            true
        );

        zTop = zBottom;
        if (reachedBottom)
            break;
    }

    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    if (std::holds_alternative<Detect>(config)) {
        auto* sd = new SensitiveDetector("MeasurementSD");
        G4SDManager::GetSDMpointer()->AddNewDetector(sd);
        logicalDetector->SetSensitiveDetector(sd);
    }
}