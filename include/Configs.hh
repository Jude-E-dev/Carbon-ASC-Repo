#pragma once

#include <string>
#include <variant>

struct SoilConfig {
    double soilCarbonPercentage;
    int soilCarbonSubdivisions;
    double soilDepth;
    double soilWidth;
};

struct CharacteriseSoilDebug {
    /// @brief Can be "csv", "hdf5", "root"
    std::string filetype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
};

struct CharacteriseSoilBinary {
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
};

struct ReplayEmissions {
    /// @brief Path to .bin file containing simulated data to replay
    std::string spectraPath;
    /// @brief Can be "csv", "hdf5", "root"
    std::string outputFiletype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
};

using GCConfig =
    std::variant<CharacteriseSoilDebug, CharacteriseSoilBinary, ReplayEmissions>;