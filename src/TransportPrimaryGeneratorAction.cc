#include "TransportPrimaryGeneratorAction.hh"

#include <CLHEP/Units/SystemOfUnits.h>
#include <math.h>

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
#include "GCConstants.hh"
#include "RunAction.hh"

TransportPrimaryGeneratorAction::TransportPrimaryGeneratorAction(
    const std::string& distributionPath,
    const SoilConfig& soil,
    EventAction* eventAction,
    RunAction* runAction
)
    : soil {soil}
    , particleTable {G4ParticleTable::GetParticleTable()}
    , gun {new G4ParticleGun}
    , binFile(distributionPath, std::ios::binary | std::ios::in | std::ios::ate)
    , rng {std::random_device {}()}
    , azimuthDist(0.0, CLHEP::twopi)
    , positionDist(0.0, 1.0)
    , eventsProcessed {0}
    , binFileExposure {0.0}
    , eventAction {eventAction}
    , runAction {runAction}
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

bool TransportPrimaryGeneratorAction::readEntry(TransportGammaEntry* out)
{
    binFile.read(reinterpret_cast<char*>(&out->energy), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->momentumZ), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->z), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->sourcePDG), sizeof(G4int));
    return binFile.good();
}

void TransportPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    if (!binFile) {
        throw std::runtime_error(
            "Tried to generate primaries but binFile invalid."
        );
    }

    TransportGammaEntry entry {};

    if (!readEntry(&entry)) {
        if (!binFile.eof()) {
            throw std::runtime_error("Binary spectrum file read error");
        }
        binFile.clear();
        binFile.seekg(sizeof(G4double), std::ios::beg);
        if (!readEntry(&entry)) {
            throw std::runtime_error("Failed to read entry after wrap-around");
        }
    }

    // Uniform position distribution on a disk
    const G4double rho = soil.soilWidth / 2.0 * std::sqrt(positionDist(rng));
    const G4double phi = azimuthDist(rng);
    const G4double pPhi = azimuthDist(rng);
    const G4double pRho = std::sqrt(1.0 - entry.momentumZ * entry.momentumZ);

    G4ThreeVector pos;
    G4ParticleMomentum momentum;
    pos.setRhoPhiZ(rho, phi, entry.z);
    momentum.setRhoPhiZ(pRho, pPhi, entry.momentumZ);

    eventAction
        ->SetSoilSourceProperties(entry.z, rho, entry.energy, entry.sourcePDG);
    runAction->IncrementTransportExposure(binFileExposure);

    gun->SetParticleDefinition(particleTable->FindParticle(GAMMA_PDG));
    gun->SetParticleEnergy(entry.energy);
    gun->SetParticleMomentumDirection(momentum);
    gun->SetParticlePosition(pos);
    gun->SetParticleTime(0.0);

    gun->GeneratePrimaryVertex(event);
    eventsProcessed++;
}

G4double TransportPrimaryGeneratorAction::GetExposureTime() const
{
    return binFileExposure * ((G4double)eventsProcessed / binFileNumEvents);
}
