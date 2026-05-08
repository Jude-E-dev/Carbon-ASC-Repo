#include "RunAction.hh"

#include <G4AnalysisManager.hh>
#include <G4RunManager.hh>
#include <G4ios.hh>
#include <filesystem>
#include <fstream>
#include <random>

#include "CRYPrimaryGeneratorAction.hh"
#include "Configs.hh"
#include "overloaded.hh"

RunAction::RunAction(GCConfig config) :
    config {config},
    binEntries(),
    binFile() {
    std::visit(
        overloaded {
            [](const CharacteriseSoilDebug& conf) {
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->SetDefaultFileType(conf.filetype);
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
                analysisManager->CreateNtupleSColumn("source");
                analysisManager->FinishNtuple();
            },
            [](const ReplayEmissions& conf) {
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->SetDefaultFileType(conf.outputFiletype);

                analysisManager->CreateNtuple(
                    "DetectorSteps",
                    "Measurement Spectrum from Detector per Step"
                );
                analysisManager->CreateNtupleDColumn(0, "E");
                analysisManager->CreateNtupleDColumn(0, "Edep");
                analysisManager->CreateNtupleSColumn(0, "name");
                analysisManager->CreateNtupleDColumn(0, "sourceDepth");
                analysisManager->CreateNtupleDColumn(0, "sourceRadius");
                analysisManager->CreateNtupleDColumn(0, "sourceE");
                analysisManager->CreateNtupleIColumn(0, "sourcePDG");
                analysisManager->FinishNtuple(0);

                analysisManager->CreateNtuple(
                    "DetectorPulses",
                    "Measurement Spectrum from Detector per Event"
                );
                analysisManager->CreateNtupleDColumn(1, "Edep");
                analysisManager->CreateNtupleDColumn(1, "sourceDepth");
                analysisManager->CreateNtupleDColumn(1, "sourceRadius");
                analysisManager->CreateNtupleDColumn(1, "sourceE");
                analysisManager->CreateNtupleIColumn(1, "sourcePDG");
                analysisManager->FinishNtuple(1);
            },
            [](const CharacteriseSoilBinary& conf) {}
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
            [](const CharacteriseSoilDebug& conf) {
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
            [](const ReplayEmissions& conf) {
                std::filesystem::create_directories(conf.outputDirectoryName);
                auto analysisManager = G4AnalysisManager::Instance();
                std::random_device rd;
                analysisManager->OpenFile(
                    conf.outputDirectoryName + "/out" + std::to_string(rd())
                );
            }
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

                auto* generatorAction =
                    (CRYPrimaryGeneratorAction*)G4RunManager::GetRunManager()
                        ->GetUserPrimaryGeneratorAction();

                G4cout << "\nExposure time: "
                       << generatorAction->GetExposureTime() << G4endl;
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
                        binFile.write(
                            reinterpret_cast<const char*>(&entry.sourcePDG),
                            sizeof(entry.sourcePDG)
                        );
                    }
                    binFile.close();
                }

                auto* generatorAction =
                    (CRYPrimaryGeneratorAction*)G4RunManager::GetRunManager()
                        ->GetUserPrimaryGeneratorAction();

                G4cout << "Exposure time: "
                       << generatorAction->GetExposureTime() << G4endl;
            },
            [](const ReplayEmissions& conf) {
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->Write();
                analysisManager->CloseFile();
            }
        },
        config
    );

    G4cout << "Run Finished. Elapsed: " << elapsed.count() << "s" << G4endl;
}

void RunAction::TallyGammaBinary(
    double energy,
    double momentumZ,
    double depth,
    int sourcePDG
) {
    GammaEntry entry;
    entry.energy = energy;
    entry.momentumZ = momentumZ;
    entry.depth = depth;
    entry.sourcePDG = sourcePDG;
    binEntries.push_back(entry);
}
