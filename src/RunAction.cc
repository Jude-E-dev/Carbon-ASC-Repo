#include "RunAction.hh"

#include <G4AnalysisManager.hh>

#include <G4ios.hh>
// #include <G4Hdf5AnalysisManager.hh>

RunAction::RunAction() {
    // auto analysisManager = G4Hdf5AnalysisManager::Instance();
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("hdf5");
    analysisManager->CreateNtuple("SecondaryGammas", "Secondary Gammas at First Step");
    analysisManager->CreateNtupleDColumn("E");
    analysisManager->CreateNtupleDColumn("p_x");
    analysisManager->CreateNtupleDColumn("p_y");
    analysisManager->CreateNtupleDColumn("p_z");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleSColumn("name");
    analysisManager->CreateNtupleSColumn("creator");
    analysisManager->CreateNtupleDColumn("time");
    analysisManager->CreateNtupleIColumn("step");
    analysisManager->CreateNtupleIColumn("trackID");
    analysisManager->FinishNtuple();
}

RunAction::~RunAction() {}

// Saving data
void RunAction::BeginOfRunAction(const G4Run* aRun) {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->OpenFile("out");

    startTime = std::chrono::steady_clock::now();
}

void RunAction::EndOfRunAction(const G4Run* aRun) {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        endTime - startTime
    );
    // extra newline since progress ticker in EventAction ends with \r
    G4cout << "\nFinished. Elapsed: " << elapsed.count() << "s" << G4endl;
}
