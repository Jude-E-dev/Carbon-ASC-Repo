#pragma once

#include <G4VUserDetectorConstruction.hh>
#include <cstddef>

#include "Configs.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class SoilConfig;

/**
 * @brief Constructs the detector setup
 * 
 */
class DetectorConstruction: public G4VUserDetectorConstruction
{
  public:
    /**
   * @brief Construct a new Detector Construction object
   * 
   * @param soil Configuration information for the soil in the scene
   * @param gcConfig Configuration information for the whole run
   */
    DetectorConstruction(const SoilConfig& soil, const GCConfig& gcConfig);

    ~DetectorConstruction() override;
    DetectorConstruction(const DetectorConstruction&) = default;
    DetectorConstruction(DetectorConstruction&&) = delete;
    DetectorConstruction& operator=(const DetectorConstruction&) = default;
    DetectorConstruction& operator=(DetectorConstruction&&) = delete;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

  private:
    G4double soilCarbonPercentage;
    std::size_t soilCarbonSubdivisions;
    G4double soilDepth;
    G4double soilWidth;
    std::vector<G4LogicalVolume*> soilLogicalLayers;
    GCConfig config;
    G4LogicalVolume* logicalDetector;
};