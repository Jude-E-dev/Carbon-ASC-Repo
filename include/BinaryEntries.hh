#pragma once

#include <G4Types.hh>

struct TransportGammaEntry {
    G4double energy;
    G4double momentumZ;
    G4double z;
    G4int sourcePDG;
};

constexpr auto TRANSPORT_GAMMA_ENTRY_SIZE =
    3 * sizeof(G4double) + 2 * sizeof(G4int);

struct PreDetectorEntry {
    G4int pdg;

    G4double z;
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

constexpr auto PRE_DETECTOR_ENTRY_SIZE =
    9 * sizeof(G4double) + 2 * sizeof(G4int);