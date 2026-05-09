#include "DetectionPrimaryGeneratorAction.hh"

#include <CLHEP/Vector/ThreeVector.h>

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleMomentum.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4ThreeVector.hh>
#include <G4ios.hh>
#include <cstdlib>

#include "EventAction.hh"
#include "RunAction.hh"

DetectionPrimaryGeneratorAction::DetectionPrimaryGeneratorAction(
    const std::string& distributionPath
) :
    gun(0),
    binFile(distributionPath, std::ios::binary | std::ios::in | std::ios::ate),
    rng(std::random_device {}()),
    azimuthDist(0.0, 2.0 * M_PI),
    eventsProcessed(0) {
    if (!binFile) {
        G4cerr << "Unable to open distribution file: " << distributionPath
               << G4endl;
        std::exit(EXIT_FAILURE);
    }

    binFileNumEvents = (static_cast<long>(binFile.tellg()) - sizeof(double))
        / (7 * sizeof(double) + 2 * sizeof(int));
    binFile.seekg(0, std::ios::beg);
    binFile.read(reinterpret_cast<char*>(&binFileExposure), sizeof(double));

    G4cout << "num events: " << binFileNumEvents
           << ", total exposure: " << binFileExposure << G4endl;

    particleTable = G4ParticleTable::GetParticleTable();
    gun = new G4ParticleGun();
}

DetectionPrimaryGeneratorAction::~DetectionPrimaryGeneratorAction() {
    if (binFile) {
        binFile.close();
    }
    delete gun;
}

void DetectionPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev) {
    if (binFile) {
        auto* runManager = G4RunManager::GetRunManager();
        auto* eventAction = (EventAction*)runManager->GetUserEventAction();
        auto* runAction = (RunAction*)runManager->GetUserRunAction();

        int pdg, sourcePDG;
        double z, rho, energy, momentumZ, momentumRho, momentumPhiDiff,
            energyInSoil, zInSoil, rhoInSoil;

        bool wasEOF = false;
        do {
            binFile.read(reinterpret_cast<char*>(&pdg), sizeof(int));
            binFile.read(reinterpret_cast<char*>(&z), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&rho), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&energy), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&momentumZ), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&momentumRho), sizeof(double));
            binFile.read(
                reinterpret_cast<char*>(&momentumPhiDiff),
                sizeof(double)
            );
            binFile.read(reinterpret_cast<char*>(&sourcePDG), sizeof(int));
            binFile.read(
                reinterpret_cast<char*>(&energyInSoil),
                sizeof(double)
            );
            binFile.read(reinterpret_cast<char*>(&zInSoil), sizeof(double));
            binFile.read(reinterpret_cast<char*>(&rhoInSoil), sizeof(double));

            wasEOF = binFile.eof();
            if (wasEOF) {
                binFile.clear();
                binFile.seekg(
                    sizeof(double), // skip exposure time
                    std::ios::beg
                );
            }
        } while (wasEOF);

        // Uniform distribution around cylinder axis
        G4double phi = azimuthDist(rng);

        eventAction->SetDetectionProperties(energyInSoil, sourcePDG, zInSoil, rhoInSoil);
        runAction->IncrementDetectionExposure(binFileExposure);

        auto momentum = G4ParticleMomentum();
        momentum.setRho(momentumRho);
        momentum.setPhi(phi + momentumPhiDiff);
        momentum.setZ(momentumZ);

        auto pos = G4ThreeVector();
        pos.setRho(rho);
        pos.setPhi(phi);
        pos.setZ(z);

        gun->SetParticleDefinition(particleTable->FindParticle(pdg));
        gun->SetParticleEnergy(energy);
        gun->SetParticleMomentumDirection(momentum);
        gun->SetParticlePosition(pos);
        gun->SetParticleTime(0.0);

        gun->GeneratePrimaryVertex(ev);
        eventsProcessed++;
    }
}

double DetectionPrimaryGeneratorAction::GetExposureTime() {
    return binFileExposure * ((double)eventsProcessed / binFileNumEvents);
}
