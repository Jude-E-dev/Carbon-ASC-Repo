#pragma once

#include <G4LogicalVolume.hh>
#include <G4VUserDetectorConstruction.hh>

#include "v4/SoilModel.hh"
#include "v4/V4Options.hh"

/**
 * @brief Geometry of the v4 modes: a laterally large soil column (layers from
 * a SoilModel) under an air volume, in a vacuum world. The germanium detector
 * is the existing one (DetectorConstruction.cc: r 8.3 cm, h 16.8 cm, centre
 * 20 cm above the ground) and is placed in transport and analog modes. Analog
 * mode also places a massless-in-effect air "tag" volume above the detector
 * to record shower particles for veto studies.
 */
class V4DetectorConstruction: public G4VUserDetectorConstruction
{
  public:
    V4DetectorConstruction(const SoilModel& soil, const V4Options& options);
    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    static constexpr G4double DETECTOR_RADIUS_CM = 8.3;
    static constexpr G4double DETECTOR_HEIGHT_CM = 16.8;
    static constexpr G4double DETECTOR_ELEVATION_CM = 20.0;
    static constexpr G4double TAG_ELEVATION_CM = 45.0;
    static constexpr G4double TAG_HALF_WIDTH_CM = 30.0;

  private:
    const SoilModel& soil;
    const V4Options& options;
    G4LogicalVolume* logicalDetector = nullptr;
};
