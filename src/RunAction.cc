#include "RunAction.hh"

#include <G4AnalysisManager.hh>

#include <G4ios.hh>

RunAction::RunAction() {
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("csv");
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
    G4cout << "Starting: " << startTime.time_since_epoch().count() << G4endl;
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
