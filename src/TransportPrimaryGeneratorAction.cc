#include "TransportPrimaryGeneratorAction.hh"

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleMomentum.hh>
#include <G4ParticleTable.hh>
#include <G4RunManager.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>
#include <G4Types.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <G4ios.hh>
#include <cmath>
#include <cstdlib>

#include "BinaryEntries.hh"
#include "Configs.hh"
#include "EventAction.hh"
#include "GCConstants.hh"
#include "RunAction.hh"

TransportPrimaryGeneratorAction::TransportPrimaryGeneratorAction(
    const std::string& distributionPath,
    const SoilConfig& soil
)
    : G4VUserPrimaryGeneratorAction {}
    , soil {soil}
    , particleTable {G4ParticleTable::GetParticleTable()}
    , gun {new G4ParticleGun}
    , binFile(distributionPath, std::ios::binary | std::ios::in | std::ios::ate)
    , rng {std::random_device {}()}
    , azimuthDist(0.0, 2.0 * M_PI)
    , positionDist(0.0, 1.0)
    , eventsProcessed {0}
{
    if (!binFile) {
        G4cerr << "Unable to open distribution file: " << distributionPath
               << G4endl;
        std::exit(EXIT_FAILURE);
    }

    binFileNumEvents = (static_cast<G4long>(binFile.tellg()) - sizeof(G4double))
        / TRANSPORT_GAMMA_ENTRY_SIZE;
    binFile.seekg(0, std::ios::beg);
    binFile.read(reinterpret_cast<char*>(&binFileExposure), sizeof(G4double));
}

TransportPrimaryGeneratorAction::~TransportPrimaryGeneratorAction()
{
    if (binFile) {
        binFile.close();
    }
    delete gun;
}

void TransportPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev)
{
    if (binFile) {
        const auto* runManager = G4RunManager::GetRunManager();
        auto* eventAction = (EventAction*)runManager->GetUserEventAction();
        auto* runAction = (RunAction*)runManager->GetUserRunAction();

        G4double energy, pZ, z;
        G4int sourcePDG;
        bool wasEOF = false;
        do {
            binFile.read(reinterpret_cast<char*>(&energy), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&pZ), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&z), sizeof(G4double));
            binFile.read(reinterpret_cast<char*>(&sourcePDG), sizeof(G4int));
            wasEOF = binFile.eof();
            if (wasEOF) {
                binFile.clear();
                binFile.seekg(
                    sizeof(G4double), // skip exposure time
                    std::ios::beg
                );
            }
        } while (z < -soil.soilDepth || wasEOF);

        // Uniform position distribution on a disk
        const G4double rho =
            soil.soilWidth / 2.0 * std::sqrt(positionDist(rng));
        const G4double phi = azimuthDist(rng);
        const G4double pPhi = azimuthDist(rng);
        const G4double pRho = std::sqrt(1.0 - pZ * pZ);

        G4ThreeVector pos;
        pos.setRhoPhiZ(rho, phi, z);
        G4ParticleMomentum momentum;
        momentum.setRhoPhiZ(pRho, pPhi, pZ);

        eventAction->SetSoilSourceProperties(z, rho, energy, sourcePDG);
        runAction->IncrementTransportExposure(binFileExposure);

        gun->SetParticleDefinition(particleTable->FindParticle(GAMMA_PDG));
        gun->SetParticleEnergy(energy);
        gun->SetParticleMomentumDirection(momentum);
        gun->SetParticlePosition(pos);
        gun->SetParticleTime(0.0);

        gun->GeneratePrimaryVertex(ev);
        eventsProcessed++;
    }
}

G4double TransportPrimaryGeneratorAction::GetExposureTime() const
{
    return binFileExposure * ((G4double)eventsProcessed / binFileNumEvents);
}
