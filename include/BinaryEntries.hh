#pragma once

#include <G4Types.hh>

/**
 * @brief An entry containing information about a gamma ray to be transported in a later step.
 * 
 */
struct TransportGammaEntry
{
    G4double energy;
    G4double momentumZ;
    G4double z;
    G4int sourcePDG;
};

/**
 * @brief The number of bytes in a TransportGammaEntry as stored in .bin files.
 * 
 */
constexpr auto TRANSPORT_GAMMA_ENTRY_SIZE =
    3 * sizeof(G4double) + 2 * sizeof(G4int);

/**
 * @brief An entry containing information about a particle that is about to intersect the detector.
 * 
 */
struct PreDetectorEntry
{
    G4int pdg;

    G4double depth;
    G4double rho;

    G4double energy;
    G4double momentumZ;
    G4double momentumRho;
    G4double momentumPhiDiff;

    G4int sourcePDG;
    G4double energyInSoil;
    G4double zInSoil;
    G4double rhoInSoil;
};

/**
 * @brief The number of bytes in a PreDetectorEntry as stored in .bin files.
 * 
 */
constexpr auto PRE_DETECTOR_ENTRY_SIZE =
    9 * sizeof(G4double) + 2 * sizeof(G4int);