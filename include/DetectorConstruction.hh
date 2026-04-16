#pragma once

#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class SoilConfig;

class DetectorConstruction: public G4VUserDetectorConstruction {
  public:
    DetectorConstruction(
        SoilConfig soil
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