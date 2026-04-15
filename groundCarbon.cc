#include <G4RandomTools.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIExecutive.hh>
#include <G4VisExecutive.hh>
#include <random>

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "GCPhysicsList.hh"

int main(int argc, char** argv) {
    G4UIExecutive* ui = nullptr;
    if (argc == 1) {
        ui = new G4UIExecutive(argc, argv);
    }

    std::random_device rd;
    G4Random::setTheEngine(new CLHEP::RanecuEngine);
    G4Random::setTheSeed(rd());

    auto runManager =
        G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);

    runManager->SetUserInitialization(
        new DetectorConstruction(0.15, 256, 0.001 * mm, 50 * m, 50 * m)
    );
    runManager->SetUserInitialization(new GCPhysicsList(true));
    runManager->SetUserInitialization(new ActionInitialization());

    auto visManager = new G4VisExecutive(argc, argv);
    visManager->Initialize();

    auto UImanager = G4UImanager::GetUIpointer();
    UImanager->ApplyCommand("/control/macroPath macros");

    if (!ui) {
        // batch mode
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    } else {
        // interactive mode
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
}