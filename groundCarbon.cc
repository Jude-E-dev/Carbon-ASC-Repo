/**
 * @file groundCarbon.cc
 * @author Jay Johnston (jaidyn.johnston@icloud.com)
 * @brief This program simulates gamma ray spectra measured by a detector
 * above soil with varying amounts of ground carbon. The program has three
 * modes it can run in:
 *   1. 'soil-character' - cosmic rays are sent into the soil, and any
 *       secondary gammas are recorded and killed.
 *   2. 'transport' - recorded gammas are replayed, and any particles that hit
 *       the detector are recorded and killed just before entering the detector.
 *   3. 'detect' - transported gamma rays are replayed directly into the
 *       detector to simulate the detector's response.
 * At each step, either binary files can be created, CSV files can be created,
 * or both.
 * 
 * @version 0.1
 * @date 2026-05-09
 * 
 */

#include <G4RandomTools.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4Types.hh>
#include <G4UIExecutive.hh>
#include <G4VisExecutive.hh>
#include <G4ios.hh>
#include <cstddef>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "ActionInitialization.hh"
#include "Configs.hh"
#include "DetectorConstruction.hh"
#include "GCPhysicsList.hh"

int invalidArguments();
void mergeBinaries(const std::vector<std::string>& args);

int main(int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);

    /* --------------------------------- Config --------------------------------- */

    // arg parse spaghetti :)

    if (args.size() < 2) {
        return invalidArguments();
    }

    if (args[1] == "merge") {
        mergeBinaries(args);
        return 0;
    }

    auto soilConfig = SoilConfig {0.15, 16, 50 * cm, 5 * m}; // smaller volume
    std::string num_runs = "";
    G4int updatePeriod = 1000;
    OutputTypes outputTypes;
    if (args[2] == "info") {
        outputTypes = OutputTypes::INFO;
    }
    else if (args[2] == "binary") {
        outputTypes = OutputTypes::BINARY;
    }
    else if (args[2] == "all") {
        outputTypes = OutputTypes::ALL;
    }
    else {
        return invalidArguments();
    }

    GCConfig gcConfig;
    if (args[1] == "soil-character") {
        gcConfig =
            CharacteriseSoilGammas {"csv", "soil-character", outputTypes};
        if (args.size() == 4) {
            num_runs = args[3];
        }
    }
    else if (args[1] == "transport") {
        updatePeriod = 50000;
        G4cout << "args: " << args.size() << G4endl;
        if (args.size() < 4)
            return invalidArguments();
        gcConfig = TransportGammas {args[3], "csv", "transport", outputTypes};
        if (args.size() == 5) {
            num_runs = args[4];
        }
    }
    else if (args[1] == "detect") {
        updatePeriod = 50000;
        if (args.size() < 4)
            return invalidArguments();
        gcConfig = Detect {args[3], "csv", "detector", outputTypes};
        if (args.size() == 5) {
            num_runs = args[4];
        }
    }
    else {
        return invalidArguments();
    }

    /* ------------------------------ Geant4 Setup ------------------------------ */

    G4UIExecutive* ui = nullptr;
    if (num_runs == "") {
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
        UImanager->ApplyCommand("/run/initialize ");
        UImanager->ApplyCommand("/run/beamOn " + num_runs);
    }
    else {
        // interactive mode
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
}


/**
 * @brief Merge binary outputs from runs. The first 8 bytes of these files
 * contain the exposure time of the run; this will handle adding these together
 * before concatenating the raw bytes
 * 
 * @param args The arguments to the program
 */
void mergeBinaries(const std::vector<std::string>& args)
{
    // Calculate total exposure
    G4double totalExposure = 0.0;
    for (size_t i = 3; i < args.size(); i++) {
        std::ifstream spectrum(args[i], std::ios::in | std::ios::binary);
        if (!spectrum.is_open())
            continue;

        G4double exposure;
        spectrum.read(reinterpret_cast<char*>(&exposure), sizeof(G4double));
        totalExposure += exposure;
        spectrum.close();
    }

    // Merge files
    std::ofstream out(args[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(&totalExposure), sizeof(G4double));
    for (size_t i = 3; i < args.size(); i++) {
        std::ifstream spectrum(args[i], std::ios::in | std::ios::binary);
        if (!spectrum.is_open())
            continue;

        spectrum.seekg(sizeof(G4double), std::ios::beg);

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

int invalidArguments()
{
    G4cerr
        << "Usage: ./groundCarbon [soil-character|transport|detect] [info|binary|all] <path.bin (transport/detect only)> <run count (optional)>\n"
        << "Or: ./groundCarbon merge <output_path.bin> output1.bin output2.bin ...\n\n"
        << "Note: if no run count is given, Geant4 will launch a GUI.\n"
        << "Note: choosing \'info\' will output a csv, \'binary\' will output a binary file.\n"
        << "      \'all\' will do both. These options have no effect when doing a \'detect\' run.\n\n"
        << "An example of how to use this:\n"
        << "  $ mpirun -n 5 ./groundCarbon soil-character binary 1000000\n"
        << "  $ ./groundCarbon merge soil-character/soil.bin soil-character/*.bin\n"
        << "  $ mpirun -n 5 ./groundCarbon transport all soil-character/soil.bin 10000000\n"
        << "  $ ./groundCarbon merge transport/predetection.bin transport/*.bin\n"
        << "  $ ./groundCarbon detect info transport/predetection.bin 10000000"
        << G4endl;
    return 1;
}