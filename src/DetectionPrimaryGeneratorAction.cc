#include "DetectionPrimaryGeneratorAction.hh"

#include <CLHEP/Vector/ThreeVector.h>

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleMomentum.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>
#include <G4Types.hh>
#include <G4ios.hh>
#include <cstdlib>

#include "BinaryEntries.hh"
#include "EventAction.hh"
#include "RunAction.hh"

DetectionPrimaryGeneratorAction::DetectionPrimaryGeneratorAction(
    const std::string& distributionPath
)
    : particleTable {G4ParticleTable::GetParticleTable()}
    , gun {new G4ParticleGun()}
    , binFile(distributionPath, std::ios::binary | std::ios::in | std::ios::ate)
    , rng {std::random_device {}()}
    , azimuthDist(0.0, 2.0 * M_PI)
    , eventsProcessed {0}
    , binFileExposure {0.0}
{
    if (!binFile) {
        G4cerr << "Unable to open distribution file: " << distributionPath
               << G4endl;
        std::exit(EXIT_FAILURE);
    }

    binFileNumEvents = (static_cast<G4long>(binFile.tellg()) - sizeof(G4double))
        / PRE_DETECTOR_ENTRY_SIZE;
    binFile.seekg(0, std::ios::beg);
    binFile.read(reinterpret_cast<char*>(&binFileExposure), sizeof(G4double));
}

DetectionPrimaryGeneratorAction::~DetectionPrimaryGeneratorAction()
{
    if (binFile) {
        binFile.close();
    }
    delete gun;
}

void DetectionPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev)
{
    if (binFile) {
        const auto* runManager = G4RunManager::GetRunManager();
        auto* eventAction = (EventAction*)runManager->GetUserEventAction();
        auto* runAction = (RunAction*)runManager->GetUserRunAction();

        G4int pdg, sourcePDG;
        G4double z, rho, energy, momentumZ, momentumRho, momentumPhiDiff,
            energyInSoil, zInSoil, rhoInSoil;

        bool wasEOF = false;
        do {
            binFile.read(reinterpret_cast<char*>(&pdg), sizeof(G4int));
            binFile.read(reinterpret_cast<char*>(&z), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&rho), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&energy), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&momentumZ), sizeof(G4double));
            binFile.read(
                reinterpret_cast<char*>(&momentumRho),
                sizeof(G4double)
            );
            binFile.read(
                reinterpret_cast<char*>(&momentumPhiDiff),
                sizeof(G4double)
            );
            binFile.read(reinterpret_cast<char*>(&sourcePDG), sizeof(G4int));
            binFile.read(
                reinterpret_cast<char*>(&energyInSoil),
                sizeof(G4double)
            );
            binFile.read(reinterpret_cast<char*>(&zInSoil), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&rhoInSoil), sizeof(G4double));

            wasEOF = binFile.eof();
            if (wasEOF) {
                binFile.clear();
                binFile.seekg(
                    sizeof(G4double), // skip exposure time
                    std::ios::beg
                );
            }
        } while (wasEOF);

        // Uniform distribution around cylinder axis
        const G4double phi = azimuthDist(rng);

        eventAction->SetDetectionProperties(
            energyInSoil,
            sourcePDG,
            zInSoil,
            rhoInSoil
        );
        runAction->IncrementDetectionExposure(binFileExposure);

        G4ThreeVector pos;
        pos.setRhoPhiZ(rho, phi, z);
        G4ParticleMomentum momentum;
        momentum.setRhoPhiZ(momentumRho, phi + momentumPhiDiff, momentumZ);

        gun->SetParticleDefinition(particleTable->FindParticle(pdg));
        gun->SetParticleEnergy(energy);
        gun->SetParticleMomentumDirection(momentum);
        gun->SetParticlePosition(pos);
        gun->SetParticleTime(0.0);

        gun->GeneratePrimaryVertex(ev);
        eventsProcessed++;
    }
}

G4double DetectionPrimaryGeneratorAction::GetExposureTime() const
{
    return binFileExposure * ((G4double)eventsProcessed / binFileNumEvents);
}
