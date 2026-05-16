#include "DetectionPrimaryGeneratorAction.hh"

#include <CLHEP/Units/SystemOfUnits.h>
#include <CLHEP/Vector/ThreeVector.h>
#include <math.h>

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
#include <stdexcept>

#include "BinaryEntries.hh"
#include "EventAction.hh"
#include "RunAction.hh"

DetectionPrimaryGeneratorAction::DetectionPrimaryGeneratorAction(
    const std::string& distributionPath,
    EventAction* eventAction,
    RunAction* runAction
)
    : eventAction {eventAction}
    , runAction {runAction}
    , particleTable {G4ParticleTable::GetParticleTable()}
    , gun {new G4ParticleGun()}
    , binFile(distributionPath, std::ios::binary | std::ios::in | std::ios::ate)
    , rng {std::random_device {}()}
    , azimuthDist(0.0, CLHEP::twopi)
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

bool DetectionPrimaryGeneratorAction::readEntry(PreDetectorEntry* out)
{
    binFile.read(reinterpret_cast<char*>(&out->pdg), sizeof(G4int));
    binFile.read(reinterpret_cast<char*>(&out->depth), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->rho), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->energy), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->momentumZ), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->momentumRho), sizeof(G4double));
    binFile.read(
        reinterpret_cast<char*>(&out->momentumPhiDiff),
        sizeof(G4double)
    );
    binFile.read(reinterpret_cast<char*>(&out->sourcePDG), sizeof(G4int));
    binFile.read(reinterpret_cast<char*>(&out->energyInSoil), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->zInSoil), sizeof(G4double));
    binFile.read(reinterpret_cast<char*>(&out->rhoInSoil), sizeof(G4double));
    return binFile.good();
}

void DetectionPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    if (!binFile) {
        throw std::runtime_error(
            "Tried to generate primaries but binFile invalid."
        );
    }

    PreDetectorEntry entry {};

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

    // Uniform distribution around cylinder axis
    const G4double phi = azimuthDist(rng);

    eventAction->SetDetectionProperties(
        entry.energyInSoil,
        entry.sourcePDG,
        entry.zInSoil,
        entry.rhoInSoil
    );
    runAction->IncrementDetectionExposure(binFileExposure);

    G4ThreeVector pos;
    pos.setRhoPhiZ(entry.rho, phi, entry.depth);
    G4ParticleMomentum momentum;
    momentum.setRhoPhiZ(
        entry.momentumRho,
        phi + entry.momentumPhiDiff,
        entry.momentumZ
    );

    gun->SetParticleDefinition(particleTable->FindParticle(entry.pdg));
    gun->SetParticleEnergy(entry.energy);
    gun->SetParticleMomentumDirection(momentum);
    gun->SetParticlePosition(pos);
    gun->SetParticleTime(0.0);

    gun->GeneratePrimaryVertex(event);
    eventsProcessed++;
}

G4double DetectionPrimaryGeneratorAction::GetExposureTime() const
{
    return binFileExposure * ((G4double)eventsProcessed / binFileNumEvents);
}
