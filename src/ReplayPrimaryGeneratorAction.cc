#include "ReplayPrimaryGeneratorAction.hh"

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleMomentum.hh>
#include <G4ParticleTable.hh>
#include <cmath>
#include <cstdlib>

#include "GCConstants.hh"

ReplayPrimaryGeneratorAction::ReplayPrimaryGeneratorAction(
    const std::string& distributionPath,
    double soilWidth
) :
    gun(0),
    binFile(distributionPath, std::ios::binary | std::ios::in),
    rng(std::random_device {}()),
    azimuthDist(0.0, 2.0 * M_PI),
    positionDist(-soilWidth / 2.0, soilWidth / 2.0) {
    if (!binFile) {
        G4cerr << "Unable to open distribution file: " << distributionPath
               << G4endl;
        std::exit(EXIT_FAILURE);
    }

    particleTable = G4ParticleTable::GetParticleTable();
    gun = new G4ParticleGun();
}

ReplayPrimaryGeneratorAction::~ReplayPrimaryGeneratorAction() {
    if (binFile) {
        binFile.close();
    }
    delete gun;
}

void ReplayPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev) {
    if (binFile) {
        double energy, pz, z;
        G4cout << "Reader at: " << binFile.tellg() << G4endl;
        binFile.read(reinterpret_cast<char*>(&energy), sizeof(double));
        binFile.read(reinterpret_cast<char*>(&pz), sizeof(double));
        binFile.read(reinterpret_cast<char*>(&z), sizeof(double));

        G4cout << "Generating particle: E = " << energy << "MeV, p_z = " << pz
               << ", z = " << z << "mm." << G4endl;

        G4double x = positionDist(rng);
        G4double y = positionDist(rng);

        G4double phi = azimuthDist(rng);
        G4double pr = std::sqrt(1.0 - pz * pz);
        G4double px = pr * std::cos(phi);
        G4double py = pr * std::sin(phi);

        gun->SetParticleDefinition(particleTable->FindParticle(GAMMA_PDG));
        gun->SetParticleEnergy(energy);
        gun->SetParticleMomentumDirection(G4ParticleMomentum(px, py, pz));
        gun->SetParticlePosition(G4ThreeVector(x, y, z));
        gun->SetParticleTime(0.0);

        gun->GeneratePrimaryVertex(ev);
    }
}
