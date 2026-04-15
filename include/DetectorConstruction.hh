#pragma once

#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

// #define N_SOIL_LAYERS 256

class DetectorConstruction: public G4VUserDetectorConstruction {
  public:
    DetectorConstruction(
        G4double soilCarbonPercentage,
        G4int numSoilLayers,
        G4double surfaceBias,
        G4double soilDepth,
        G4double soilWidth
    );
    virtual ~DetectorConstruction();
    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

  private:
    G4double soilCarbonPercentage;
    G4int numSoilLayers;
    G4double surfaceBias;
    G4double soilDepth;
    G4double soilWidth;
    std::vector<G4LogicalVolume*> soilLogicalLayers;
};