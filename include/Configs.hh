#pragma once

#include <G4String.hh>
#include <G4Types.hh>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

/**
 * @brief The configuration details of the simulation soil.
 * 
 */
struct SoilConfig
{
    G4double soilCarbonPercentage;
    /// @brief The number of subdivisions if the depth were infinite.
    std::size_t soilCarbonSubdivisions;
    G4double soilDepth;
    G4double soilWidth;
};

enum class OutputTypes : std::uint8_t
{
    INFO,
    BINARY,
    ALL
};

/**
 * @brief A run where cosmic rays are simulated, and gamma ray production is
 * captured in the soil.
 * 
 */
struct CharacteriseSoilGammas
{
    /// @brief Can be "csv", "hdf5", "root"
    std::string filetype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief Types of outputs to save.
    OutputTypes outputTypes;
};

/**
 * @brief A run where gamma rays from a previous soil characterisation run
 * are replayed and transported to the detector. Particles incident to the
 * detector are stored to be replayed again later when simulating detector
 * response.
 * 
 */
struct TransportGammas
{
    /// @brief Path to .bin file containing simulated data to transport
    std::string spectraPath;
    /// @brief Can be "csv", "hdf5", "root"
    std::string outputFiletype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief Types of outputs to save.
    OutputTypes outputTypes;
};

/**
 * @brief A run where particles from a previous transport run are replayed to
 * simulate the detector's response.
 * 
 */
struct Detect
{
    /// @brief Path to .bin file containing transported data to detect
    std::string spectraPath;
    /// @brief Can be "csv", "hdf5", "root"
    std::string outputFiletype;
    /// @brief Directory to store outputs, without trailing slash /
    std::string outputDirectoryName;
    /// @brief Types of outputs to save.
    OutputTypes outputTypes;
};

/**
 * @brief The type of run to do.
 * 
 */
using GCConfig = std::variant<CharacteriseSoilGammas, TransportGammas, Detect>;