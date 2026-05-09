#pragma once

struct TransportGammaEntry {
    double energy;
    double momentumZ;
    double z;
    int sourcePDG;
};

struct DetectorSurfaceEntry {
    int pdg;

    double z;
    double rho;

    double energy;
    double momentumZ;
    double momentumRho;
    double momentumPhiDiff;

    int sourcePDG;
    double energyInSoil;
    double zInSoil;
    double rhoInSoil;
};