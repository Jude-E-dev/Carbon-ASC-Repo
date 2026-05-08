#include <G4RandomTools.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIExecutive.hh>
#include <G4VisExecutive.hh>
#include <G4ios.hh>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "ActionInitialization.hh"
#include "Configs.hh"
#include "DetectorConstruction.hh"
#include "GCPhysicsList.hh"

int invalidArguments() {
    G4cerr
        << "Usage: ./groundCarbon [(character-debug)|(character-bin)|(replay <spectrum.bin>)] <macro.mac (optional)>\n"
        << "  E.g., ./groundCarbon character-debug characterise.mac\n"
        << "Or: ./groundCarbon merge-spectra <output_path.bin> spectrum1.bin spectrum2.bin ...\n"
        << "Note: if no macro is given, Geant4 will launch a GUI." << G4endl;
    return 1;
}

void mergeSpectra(std::vector<std::string> args);

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    /* --------------------------------- Config --------------------------------- */

    // arg parse spaghetti :)

    if (args.size() < 2) {
        return invalidArguments();
    }

    std::string macro = "";
    int updatePeriod = 1000;

    auto soilConfig = SoilConfig {0.15, 16, 50 * cm, 5 * m}; // smaller volume
    GCConfig gcConfig;
    if (args[1] == "character-debug") {
        gcConfig = CharacteriseSoilDebug {"csv", "character"};
        if (args.size() == 3) {
            macro = args[2];
        }
    } else if (args[1] == "character-bin") {
        gcConfig = CharacteriseSoilBinary {"spectra"};
        if (args.size() == 3) {
            macro = args[2];
        }
    } else if (args[1] == "replay") {
        updatePeriod = 50000;
        if (args.size() < 3)
            return invalidArguments();
        gcConfig = ReplayEmissions {args[2], "csv", "replay"};
        if (args.size() == 4) {
            macro = args[3];
        }
    } else if (args[1] == "merge-spectra") {
        mergeSpectra(args);
        return 0;
    } else {
        return invalidArguments();
    }

    /* ------------------------------ Geant4 Setup ------------------------------ */

    G4UIExecutive* ui = nullptr;
    if (macro == "") {
        ui = new G4UIExecutive(argc, argv);
    }

    std::random_device rd;
    G4Random::setTheEngine(new CLHEP::RanecuEngine);
    G4Random::setTheSeed(rd());

    auto runManager =
        G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
    runManager->SetUserInitialization(
        new DetectorConstruction(soilConfig, gcConfig)
    );
    runManager->SetUserInitialization(new GCPhysicsList());
    runManager->SetUserInitialization(
        new ActionInitialization(soilConfig, gcConfig, updatePeriod)
    );

    auto visManager = new G4VisExecutive(argc, argv);
    visManager->Initialize();

    auto UImanager = G4UImanager::GetUIpointer();
    UImanager->ApplyCommand("/control/macroPath macros");

    if (!ui) {
        // batch mode
        UImanager->ApplyCommand("/control/execute " + macro);
    } else {
        // interactive mode
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
}

void mergeSpectra(std::vector<std::string> args) {
    // Calculate total exposure
    double totalExposure = 0.0;
    for (int i = 3; i < args.size(); i++) {
        std::ifstream spectrum(args[i], std::ios::in | std::ios::binary);
        if (!spectrum.is_open())
            continue;

        double exposure;
        spectrum.read(reinterpret_cast<char*>(&exposure), sizeof(double));
        totalExposure += exposure;
        spectrum.close();
    }

    // Merge files
    std::ofstream out(args[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(&totalExposure), sizeof(double));
    for (int i = 3; i < args.size(); i++) {
        std::ifstream spectrum(args[i], std::ios::in | std::ios::binary);
        if (!spectrum.is_open())
            continue;

        spectrum.seekg(sizeof(double), std::ios::beg);

        char buffer[65536];
        while (spectrum.read(buffer, sizeof(buffer))) {
            out.write(buffer, spectrum.gcount());
        }
        if (spectrum.gcount() > 0) {
            out.write(buffer, spectrum.gcount());
        }
    }
    out.close();
}