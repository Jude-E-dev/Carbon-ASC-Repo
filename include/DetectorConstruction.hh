#pragma once

#include "Configs.hh"
#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class SoilConfig;

class DetectorConstruction: public G4VUserDetectorConstruction {
  public:
    DetectorConstruction(SoilConfig soil, GCConfig gcConfig);
    virtual ~DetectorConstruction();
    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

  private:
    G4double soilCarbonPercentage;
    G4int soilCarbonSubdivisions;
    G4double soilDepth;
    G4double soilWidth;
    std::vector<G4LogicalVolume*> soilLogicalLayers;
    GCConfig config;
    G4LogicalVolume* logicalDetector;
};