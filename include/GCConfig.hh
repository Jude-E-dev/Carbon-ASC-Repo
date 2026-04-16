#pragma once

#include <string>
#include <variant>

struct SoilConfig {
    double soilCarbonPercentage;
    int numSoilLayers;
    double surfaceBias;
    double soilDepth;
    double soilWidth;
};

struct CharacteriseSoilDebug {
    /// @brief Can be "csv", "hdf5", "root"
    std::string fileType;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief How to set up the soil
    SoilConfig soilConfig;
};

struct CharacteriseSoilBinary {
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief How to set up the soil
    SoilConfig soilConfig;
};

struct ReplayEmissions {
    /// @brief Path to .bin file containing simulated data to replay
    std::string spectraPath;
    /// @brief How to set up the soil
    SoilConfig soilConfig;
};

using GCConfig =
    std::variant<CharacteriseSoilDebug, CharacteriseSoilBinary, ReplayEmissions>;