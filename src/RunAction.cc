#include "RunAction.hh"

#include <G4AnalysisManager.hh>
#include <G4ios.hh>
#include <filesystem>
#include <fstream>
#include <random>

#include "overloaded.hh"

RunAction::RunAction(GCConfig config) : config {config}, binEntries(), binFile() {
    std::visit(
        overloaded {
            [](const CharacteriseSoilDebug& conf) {
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->SetDefaultFileType(conf.fileType);
                analysisManager->CreateNtuple(
                    "SecondaryGammas",
                    "Secondary Gammas at First Step"
                );
                analysisManager->CreateNtupleDColumn("E");
                analysisManager->CreateNtupleDColumn("p_x");
                analysisManager->CreateNtupleDColumn("p_y");
                analysisManager->CreateNtupleDColumn("p_z");
                analysisManager->CreateNtupleDColumn("x");
                analysisManager->CreateNtupleDColumn("y");
                analysisManager->CreateNtupleDColumn("z");
                analysisManager->CreateNtupleSColumn("creator");
                analysisManager->CreateNtupleDColumn("time");
                analysisManager->FinishNtuple();
            },
            [](auto const&) {}
        },
        config
    );
}

RunAction::~RunAction() {}

// Saving data
void RunAction::BeginOfRunAction(const G4Run* aRun) {
    startTime = std::chrono::steady_clock::now();
    std::visit(
        overloaded {
            [this](const CharacteriseSoilDebug& conf) {
                std::filesystem::create_directories(conf.outputDirectoryName);
                auto analysisManager = G4AnalysisManager::Instance();
                std::random_device rd;
                analysisManager->OpenFile(
                    conf.outputDirectoryName + "/out" + std::to_string(rd())
                );
            },
            [this](const CharacteriseSoilBinary& conf) {
                std::filesystem::create_directories(conf.outputDirectoryName);
                std::random_device rd;
                binFile = std::ofstream(
                    conf.outputDirectoryName + "/out" + std::to_string(rd())
                        + ".bin",
                    std::ios::app | std::ios::binary
                );
                binEntries.clear();
            },
            [](auto const&) {}
        },
        config
    );
}

void RunAction::EndOfRunAction(const G4Run* aRun) {
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        endTime - startTime
    );

    std::visit(
        overloaded {
            [](const CharacteriseSoilDebug& conf) {
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->Write();
                analysisManager->CloseFile();
            },
            [this](const CharacteriseSoilBinary& conf) {
                if (binFile.is_open()) {
                    for (GammaEntry entry : binEntries) {
                        binFile.write(
                            reinterpret_cast<const char*>(&entry.energy),
                            sizeof(entry.energy)
                        );
                        binFile.write(
                            reinterpret_cast<const char*>(&entry.momentumZ),
                            sizeof(entry.momentumZ)
                        );
                        binFile.write(
                            reinterpret_cast<const char*>(&entry.depth),
                            sizeof(entry.depth)
                        );
                    }
                    binFile.close();
                }
            },
            [](auto const&) {}
        },
        config
    );

    // extra newline since progress ticker in EventAction ends with \r
    G4cout << "\nRun Finished. Elapsed: " << elapsed.count() << "s" << G4endl;
}

void RunAction::TallyGammaBin(double energy, double momentumZ, double depth) {
    GammaEntry entry;
    entry.energy = energy;
    entry.momentumZ = momentumZ;
    entry.depth = depth;
    binEntries.push_back(entry);
}
