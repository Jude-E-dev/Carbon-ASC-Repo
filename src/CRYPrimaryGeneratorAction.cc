#include "CRYPrimaryGeneratorAction.hh"

#include <fstream>
#include <CRYSetup.h>
#include <CLHEP/Random/Random.h>

#include <G4Event.hh>
#include <G4SystemOfUnits.hh>

#include "GCConstants.hh"
#include "RNGWrapper.hh"

//----------------------------------------------------------------------------//
CRYPrimaryGeneratorAction::CRYPrimaryGeneratorAction(const char* inputfile) {
    // define a particle gun
    particleGun = new G4ParticleGun();

    // Read the cry input file
    std::ifstream inputFile;
    inputFile.open(inputfile, std::ios::in);
    char buffer[1000];

    if (inputFile.fail()) {
        if (*inputfile != 0) //....only complain if a filename was given
            G4cout << "PrimaryGeneratorAction: Failed to open CRY input file= "
                   << inputfile << G4endl;
        InputState = -1;
    } else {
        std::string setupString("");
        while (!inputFile.getline(buffer, 1000).eof()) {
            setupString.append(buffer);
            setupString.append(" ");
        }

        CRYSetup* setup = new CRYSetup(setupString, CRY_DATA_PATH);

        gen = new CRYGenerator(setup);

        // set random number generator
        RNGWrapper<CLHEP::HepRandomEngine>::set(
            CLHEP::HepRandom::getTheEngine(),
            &CLHEP::HepRandomEngine::flat
        );
        setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
        InputState = 0;
    }
    // create a vector to store the CRY particle properties
    vect = new std::vector<CRYParticle*>;

    // Create the table containing all particle names
    particleTable = G4ParticleTable::GetParticleTable();

    // Create the messenger file
    gunMessenger = new CRYPrimaryGeneratorMessenger(this);
}

//----------------------------------------------------------------------------//
CRYPrimaryGeneratorAction::~CRYPrimaryGeneratorAction() {}

//----------------------------------------------------------------------------//
void CRYPrimaryGeneratorAction::InputCRY() {
    InputState = 1;
}

//----------------------------------------------------------------------------//
void CRYPrimaryGeneratorAction::UpdateCRY(std::string* MessInput) {
    CRYSetup* setup = new CRYSetup(*MessInput, CRY_DATA_PATH);

    gen = new CRYGenerator(setup);

    // set random number generator
    RNGWrapper<CLHEP::HepRandomEngine>::set(
        CLHEP::HepRandom::getTheEngine(),
        &CLHEP::HepRandomEngine::flat
    );
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
    InputState = 0;
}

//----------------------------------------------------------------------------//
void CRYPrimaryGeneratorAction::CRYFromFile(G4String newValue) {
    // Read the cry input file
    std::ifstream inputFile;
    inputFile.open(newValue, std::ios::in);
    char buffer[1000];

    if (inputFile.fail()) {
        G4cout << "Failed to open input file " << newValue << G4endl;
        G4cout << "Make sure to define the cry library on the command line"
               << G4endl;
        InputState = -1;
    } else {
        std::string setupString("");
        while (!inputFile.getline(buffer, 1000).eof()) {
            setupString.append(buffer);
            setupString.append(" ");
        }

        CRYSetup* setup = new CRYSetup(setupString, CRY_DATA_PATH);

        gen = new CRYGenerator(setup);

        // set random number generator
        RNGWrapper<CLHEP::HepRandomEngine>::set(
            CLHEP::HepRandom::getTheEngine(),
            &CLHEP::HepRandomEngine::flat
        );
        setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);
        InputState = 0;
    }
}

//----------------------------------------------------------------------------//
void CRYPrimaryGeneratorAction::GeneratePrimaries(G4Event* ev) {
    if (InputState != 0) {
        G4String* str =
            new G4String("CRY library was not successfully initialized");
        G4Exception("PrimaryGeneratorAction", "1", RunMustBeAborted, *str);
    }
    G4String particleName;
    vect->clear();
    gen->genEvent(vect);

    for (unsigned j = 0; j < vect->size(); j++) {
        particleName = CRYUtils::partName((*vect)[j]->id());

        particleGun->SetParticleDefinition(
            particleTable->FindParticle((*vect)[j]->PDGid())
        );
        particleGun->SetParticleEnergy((*vect)[j]->ke() * MeV);
        particleGun->SetParticlePosition(G4ThreeVector(
            (*vect)[j]->x() * m,
            (*vect)[j]->y() * m,
            (*vect)[j]->z() * m
        ));
        particleGun->SetParticleMomentumDirection(
            G4ThreeVector((*vect)[j]->u(), (*vect)[j]->v(), (*vect)[j]->w())
        );
        particleGun->SetParticleTime((*vect)[j]->t());
        particleGun->GeneratePrimaryVertex(ev);
        delete (*vect)[j];
    }
}

double CRYPrimaryGeneratorAction::GetExposureTime() {
    return gen->timeSimulated();
}