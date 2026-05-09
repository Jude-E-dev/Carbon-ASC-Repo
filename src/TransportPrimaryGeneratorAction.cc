#include "TransportPrimaryGeneratorAction.hh"

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
#include "RunAction.hh"

TransportPrimaryGeneratorAction::TransportPrimaryGeneratorAction(
    const std::string& distributionPath,
    SoilConfig soil
) :
    gun(0),
    binFile(distributionPath, std::ios::binary | std::ios::in | std::ios::ate),
    rng(std::random_device {}()),
    azimuthDist(0.0, 2.0 * M_PI),
    positionDist(0.0, 1.0),
    soil {soil},
    eventsProcessed(0) {
    if (!binFile) {
        G4cerr << "Unable to open distribution file: " << distributionPath
               << G4endl;
        std::exit(EXIT_FAILURE);
    }

    binFileNumEvents = (static_cast<long>(binFile.tellg()) - sizeof(double))
        / (3 * sizeof(double) + sizeof(int));
    binFile.seekg(0, std::ios::beg);
    binFile.read(reinterpret_cast<char*>(&binFileExposure), sizeof(double));

    G4cout << "num events: " << binFileNumEvents
           << ", total exposure: " << binFileExposure << G4endl;

    particleTable = G4ParticleTable::GetParticleTable();
    gun = new G4ParticleGun();
}

TransportPrimaryGeneratorAction::~TransportPrimaryGeneratorAction() {
    if (binFile) {
        binFile.close();
    }
    delete gun;
}

void TransportPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev) {
    if (binFile) {
        auto* runManager = G4RunManager::GetRunManager();
        auto* eventAction = (EventAction*)runManager->GetUserEventAction();
        auto* runAction = (RunAction*)runManager->GetUserRunAction();

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
                binFile.seekg(
                    sizeof(double), // skip exposure time
                    std::ios::beg
                );
            }
        } while (z < -soil.soilDepth || wasEOF);

        // Uniform distribution on a disk
        G4double rho = soil.soilWidth / 2.0 * std::sqrt(positionDist(rng));
        G4double theta = azimuthDist(rng);
        G4double x = rho * std::cos(theta);
        G4double y = rho * std::sin(theta);

        eventAction->SetSoilSourceProperties(
            z,
            std::sqrt(x * x + y * y),
            energy,
            sourcePDG
        );
        runAction->IncrementTransportExposure(binFileExposure);

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
        eventsProcessed++;
    }
}

double TransportPrimaryGeneratorAction::GetExposureTime() {
    return binFileExposure * ((double)eventsProcessed / binFileNumEvents);
}
