#include "CRYPrimaryGeneratorAction.hh"

#include <CLHEP/Random/Random.h>
#include <CRYSetup.h>

#include <G4Event.hh>
#include <G4ParticleGun.hh>
#include <G4ParticleTable.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>
#include <G4Types.hh>
#include <G4ios.hh>
#include <fstream>
#include <sstream>
#include <string_view>

#include "RNGWrapper.hh"

CRYPrimaryGeneratorAction::CRYPrimaryGeneratorAction(
    std::string_view crySetupPath,
    std::string_view cryDataPath
)
    : particleGun(new G4ParticleGun())
    , cryDataPath(cryDataPath)
    , particles(0)
    , particleTable(G4ParticleTable::GetParticleTable())
    , gunMessenger(new CRYPrimaryGeneratorMessenger(this))
{
    if (crySetupPath.empty()) {
        inputState = -1;
        return;
    }

    std::ifstream inputFile {std::string(crySetupPath)};

    if (!inputFile) {
        G4cout << "PrimaryGeneratorAction: Failed to open CRY input file: "
               << crySetupPath << G4endl;
        inputState = -1;
        return;
    }

    std::stringstream setupStream;
    std::string line;
    // Cry setup parser is picky
    while (std::getline(inputFile, line)) {
        setupStream << line << ' ';
    }
    const auto setupString = setupStream.str();

    auto* setup = new CRYSetup(setupString, std::string(cryDataPath));
    gen = new CRYGenerator(setup);

    RNGWrapper<CLHEP::HepRandomEngine>::set(
        CLHEP::HepRandom::getTheEngine(),
        &CLHEP::HepRandomEngine::flat
    );
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);

    inputState = 0;
}

void CRYPrimaryGeneratorAction::InputCRY()
{
    inputState = 1;
}

void CRYPrimaryGeneratorAction::UpdateCRY(G4String* input)
{
    auto* setup = new CRYSetup(*input, cryDataPath);

    gen = new CRYGenerator(setup);

    // set random number generator
    RNGWrapper<CLHEP::HepRandomEngine>::set(
        CLHEP::HepRandom::getTheEngine(),
        &CLHEP::HepRandomEngine::flat
    );
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
    inputState = 0;
}

void CRYPrimaryGeneratorAction::CRYFromFile(const G4String& newPath)
{
    // Read the cry input file
    std::ifstream inputFile {newPath};

    if (!inputFile) {
        G4cout << "PrimaryGeneratorAction: Failed to open CRY input file: "
               << newPath << G4endl;
        inputState = -1;
        return;
    }

    std::stringstream setupStream;
    std::string line;
    // Cry setup parser is picky
    while (std::getline(inputFile, line)) {
        setupStream << line << ' ';
    }
    const auto setupString = setupStream.str();

    auto* setup = new CRYSetup(setupString, std::string(cryDataPath));
    gen = new CRYGenerator(setup);

    // set random number generator
    RNGWrapper<CLHEP::HepRandomEngine>::set(
        CLHEP::HepRandom::getTheEngine(),
        &CLHEP::HepRandomEngine::flat
    );
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
    inputState = 0;
}

void CRYPrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    if (inputState != 0) {
        G4String str = "CRY library was not successfully initialized";
        G4Exception("PrimaryGeneratorAction", "1", RunMustBeAborted, str);
    }
    particles.clear();
    gen->genEvent(&particles);

    for (size_t j = 0; j < particles.size(); j++) {
        particleGun->SetParticleDefinition(
            particleTable->FindParticle(particles[j]->PDGid())
        );
        particleGun->SetParticleEnergy(particles[j]->ke() * MeV);
        particleGun->SetParticlePosition(G4ThreeVector(
            particles[j]->x() * m,
            particles[j]->y() * m,
            particles[j]->z() * m
        ));
        particleGun->SetParticleMomentumDirection(G4ThreeVector(
            particles[j]->u(),
            particles[j]->v(),
            particles[j]->w()
        ));
        particleGun->SetParticleTime(particles[j]->t());
        particleGun->GeneratePrimaryVertex(event);
        delete particles[j];
    }
}

G4double CRYPrimaryGeneratorAction::GetExposureTime() const
{
    return gen->timeSimulated();
}