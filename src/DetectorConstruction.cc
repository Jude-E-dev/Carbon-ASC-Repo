#include "DetectorConstruction.hh"

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4NistManager.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4VisAttributes.hh>
#include <cmath>

DetectorConstruction::DetectorConstruction(
    G4double soilCarbonPercentage,
    G4int numSoilLayers,
    G4double surfaceBias,
    G4double soilDepth,
    G4double soilWidth
) :
    G4VUserDetectorConstruction(),
    soilLogicalLayers(numSoilLayers, nullptr),
    soilCarbonPercentage {soilCarbonPercentage},
    numSoilLayers {numSoilLayers},
    surfaceBias {surfaceBias},
    soilDepth {soilDepth},
    soilWidth {soilWidth} {}

DetectorConstruction::~DetectorConstruction() {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
    G4NistManager* nist = G4NistManager::Instance();

    /* -------------------------------- Materials ------------------------------- */

    G4Element* elO = new G4Element("Oxygen", "O", 8., 16.00 * g / mole);
    G4Element* elSi = new G4Element("Silicon", "Si", 14., 28.09 * g / mole);
    G4Element* elAl = new G4Element("Aluminum", "Al", 13., 26.98 * g / mole);
    G4Element* elFe = new G4Element("Iron", "Fe", 26., 55.85 * g / mole);
    G4Element* elCa = new G4Element("Calcium", "Ca", 20., 69.72 * g / mole);
    G4Element* elNa = new G4Element("Sodium", "Na", 11., 22.99 * g / mole);
    G4Element* elK = new G4Element("Potassium", "K", 19., 39.1 * g / mole);
    G4Element* elMg = new G4Element("Magnesium", "Mg", 12., 24.31 * g / mole);
    G4Element* elTi = new G4Element("Titanium", "Ti", 22., 47.87 * g / mole);
    G4Element* elMn = new G4Element("Manganese", "Mn", 25., 54.94 * g / mole);
    G4Element* elC = new G4Element("Carbon", "C", 6., 12.01 * g / mole);
    G4Element* elH = new G4Element("Hydrogen", "H", 1., 1.01 * g / mole);
    G4Element* elN = new G4Element("Nitrogen", "N", 7., 14.01 * g / mole);
    G4Element* elP = new G4Element("Phosphorus", "P", 15., 30.97 * g / mole);
    G4Element* elS = new G4Element("Sulfur", "S", 16., 32.07 * g / mole);

    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
    G4Material* water = nist->FindOrBuildMaterial("G4_WATER");
    G4Material* soilOrganicCarbon =
        new G4Material("soilOrganicCarbon", 1.2 * g / cm3, 6);
    soilOrganicCarbon->AddElement(elC, 0.58);
    soilOrganicCarbon->AddElement(elO, 0.22);
    soilOrganicCarbon->AddElement(elH, 0.12);
    soilOrganicCarbon->AddElement(elN, 0.06);
    soilOrganicCarbon->AddElement(elP, 0.01);
    soilOrganicCarbon->AddElement(elS, 0.01);

    // From McTaggart
    G4Material* mineralComponent =
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

    G4Box* solidWorld = new G4Box(
        "WorldSolid",
        0.5 * soilWidth,
        0.5 * soilWidth,
        soilDepth // twice as long as soil depth
    );

    G4LogicalVolume* logicWorld =
        new G4LogicalVolume(solidWorld, air, "WorldLogical");

    G4VisAttributes* visWorld =
        new G4VisAttributes(G4Colour(0.3, 0.4, 0.9, 1.0));
    visWorld->SetForceWireframe(true);
    logicWorld->SetVisAttributes(visWorld);

    G4VPhysicalVolume* physWorld = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logicWorld,
        "WorldPhysical",
        nullptr,
        false,
        0,
        true
    );

    /* ------------------------------ Soil Volumes ------------------------------ */
    /* -------------- https://www.desmos.com/calculator/tcl1fcxpzj -------------- */

    G4double airDensity = air->GetDensity() / (g / cm3);
    G4double waterDensity = water->GetDensity() / (g / cm3);
    G4double carbonDensity = soilOrganicCarbon->GetDensity() / (g / cm3);
    G4double mineralDensity = mineralComponent->GetDensity() / (g / cm3);

    // Soil parameters, from McTaggart
    G4double airVolumeFraction = 0.25;
    G4double waterVolumeFraction = 0.25;
    G4double k = 0.02349 / cm;

    // Layering parameters
    G4double growth =
        std::pow(1.0 + soilDepth / surfaceBias, 1.0 / numSoilLayers);

    // Construct layers
    G4double zTop = 0.0;
    for (int i = 0; i < numSoilLayers; i++) {
        G4double zBottom = -(std::pow(growth, i + 1) - 1) * surfaceBias;
        if (i == numSoilLayers - 1) {
            zBottom = -soilDepth;
        }

        G4double carbonVolumeFraction =
            soilCarbonPercentage * std::exp(k * zTop);
        G4double mineralVolumeFraction = 1.0 - airVolumeFraction
            - waterVolumeFraction - carbonVolumeFraction;

        G4double totalMass = airVolumeFraction * airDensity
            + waterVolumeFraction * waterDensity
            + carbonVolumeFraction * carbonDensity
            + mineralVolumeFraction * mineralDensity;

        G4double density = totalMass
            / (airVolumeFraction + waterVolumeFraction + carbonVolumeFraction
               + mineralVolumeFraction);

        G4String soilName = "SoilMixtureLayer_" + std::to_string(i);
        G4Material* soilMixture =
            new G4Material(soilName, density * g / cm3, 4);

        G4double airMassFraction = airVolumeFraction * airDensity / density;
        G4double waterMassFraction =
            waterVolumeFraction * waterDensity / density;
        G4double soilMassFraction =
            carbonVolumeFraction * carbonDensity / density;
        G4double mineralMassFraction =
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

        G4String nameSolid = "SolidLayer_" + std::to_string(i);
        G4String nameLogical = "LogicalLayer_" + std::to_string(i);
        G4String namePhysical = "PhysicalLayer_" + std::to_string(i);

        G4Box* solidLayer = new G4Box(
            nameSolid,
            0.5 * soilWidth,
            0.5 * soilWidth,
            0.5 * (zTop - zBottom)
        );

        soilLogicalLayers[i] =
            new G4LogicalVolume(solidLayer, soilMixture, nameLogical);

        G4double t = 0.8 + 0.5 * (carbonVolumeFraction / 0.15);
        G4VisAttributes* visSoil =
            new G4VisAttributes(G4Colour(0.74 * t, 0.55 * t, 0.45 * t, 0.3));
        visSoil->SetForceSolid(true);
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
    }

    return physWorld;
}

void DetectorConstruction::ConstructSDandField() {}