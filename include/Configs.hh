#pragma once

#include <string>
#include <variant>

struct SoilConfig {
    double soilCarbonPercentage;
    int soilCarbonSubdivisions;
    double soilDepth;
    double soilWidth;
};

enum class OutputTypes { INFO, BINARY, ALL };

struct CharacteriseSoilGammas {
    /// @brief Can be "csv", "hdf5", "root"
    std::string filetype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief Types of outputs to save.
    OutputTypes outputTypes;
};

struct TransportGammas {
    /// @brief Path to .bin file containing simulated data to transport
    std::string spectraPath;
    /// @brief Can be "csv", "hdf5", "root"
    std::string outputFiletype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief Types of outputs to save.
    OutputTypes outputTypes;
};

struct Detect {
    /// @brief Path to .bin file containing transported data to detect
    std::string spectraPath;
    /// @brief Can be "csv", "hdf5", "root"
    std::string outputFiletype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief Types of outputs to save.
    OutputTypes outputTypes;
};

using GCConfig = std::variant<CharacteriseSoilGammas, TransportGammas, Detect>;