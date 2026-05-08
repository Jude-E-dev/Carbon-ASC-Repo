#include "ReplayPrimaryGeneratorAction.hh"

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleMomentum.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4ios.hh>
#include <cmath>
#include <cstdlib>

#include "Configs.hh"
#include "EventAction.hh"
#include "GCConstants.hh"

ReplayPrimaryGeneratorAction::ReplayPrimaryGeneratorAction(
    const std::string& distributionPath,
    SoilConfig soil
) :
    gun(0),
    binFile(distributionPath, std::ios::binary | std::ios::in),
    rng(std::random_device {}()),
    azimuthDist(0.0, 2.0 * M_PI),
    positionDist(0.0, 1.0),
    soil {soil} {
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
        int sourcePDG;
        bool wasEOF = false;
        do {
            binFile.read(reinterpret_cast<char*>(&energy), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&pz), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&z), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&sourcePDG), sizeof(int));
            wasEOF = binFile.eof();
            if (wasEOF) {
                binFile.clear();
                binFile.seekg(0, std::ios::beg);
                G4cout
                    << "Reached end of replay .bin file. Looping to beginning."
                    << G4endl;
            }
        } while (z < -soil.soilDepth || wasEOF);

        // Uniform distribution on a disk
        G4double rho = soil.soilWidth / 2.0 * std::sqrt(positionDist(rng));
        G4double theta = azimuthDist(rng);
        G4double x = rho * std::cos(theta);
        G4double y = rho * std::sin(theta);

        auto* eventAction =
            (EventAction*)(G4RunManager::GetRunManager()->GetUserEventAction());
        eventAction->SetSourceProperties(
            z,
            std::sqrt(x * x + y * y),
            energy,
            sourcePDG
        );

        G4double pPhi = azimuthDist(rng);
        G4double pr = std::sqrt(1.0 - pz * pz);
        G4double px = pr * std::cos(pPhi);
        G4double py = pr * std::sin(pPhi);

        gun->SetParticleDefinition(particleTable->FindParticle(GAMMA_PDG));
        gun->SetParticleEnergy(energy);
        gun->SetParticleMomentumDirection(G4ParticleMomentum(px, py, pz));
        gun->SetParticlePosition(G4ThreeVector(x, y, z));
        gun->SetParticleTime(0.0);

        gun->GeneratePrimaryVertex(ev);
    }
}
