#include "RunAction.hh"

#include <G4AnalysisManager.hh>
#include <G4RunManager.hh>
#include <G4ios.hh>
#include <filesystem>
#include <fstream>
#include <random>

#include "BinaryEntries.hh"
#include "CRYPrimaryGeneratorAction.hh"
#include "Configs.hh"
#include "DetectionPrimaryGeneratorAction.hh"
#include "TransportPrimaryGeneratorAction.hh"
#include "overloaded.hh"

RunAction::RunAction(const GCConfig& config)
    : startTime {}
    , config {config}
    , soilBinEntries {}
    , soilBinFile {}
    , preDetectorBinEntries {}
    , preDetectorBinFile {}
{
    auto visitor = overloaded {
        [](const CharacteriseSoilGammas& conf) {
            if (conf.outputTypes == OutputTypes::BINARY)
                return;

            auto* analysisManager = G4AnalysisManager::Instance();
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
        [](const TransportGammas& conf) {
            if (conf.outputTypes == OutputTypes::BINARY)
                return;

            auto* analysisManager = G4AnalysisManager::Instance();
            analysisManager->SetDefaultFileType(conf.outputFiletype);

            analysisManager->CreateNtuple(
                "DetectorSteps",
                "Measurement Spectrum from Detector per Step"
            );
            analysisManager->CreateNtupleDColumn("E");
            analysisManager->CreateNtupleSColumn("name");
            analysisManager->CreateNtupleDColumn("z");
            analysisManager->CreateNtupleDColumn("rho");
            analysisManager->CreateNtupleDColumn("phi");
            analysisManager->CreateNtupleDColumn("momentumZ");
            analysisManager->CreateNtupleDColumn("momentumRho");
            analysisManager->CreateNtupleDColumn("momentumPhi");
            analysisManager->CreateNtupleDColumn("sourceZ");
            analysisManager->CreateNtupleDColumn("sourceRho");
            analysisManager->CreateNtupleDColumn("sourceE");
            analysisManager->CreateNtupleIColumn("sourcePDG");
            analysisManager->FinishNtuple();
        },
        [](const Detect& conf) {
            auto* analysisManager = G4AnalysisManager::Instance();
            analysisManager->SetDefaultFileType(conf.outputFiletype);

            analysisManager->CreateNtuple(
                "DetectorPulses",
                "Measurement Spectrum from Detector per Event"
            );
            analysisManager->CreateNtupleDColumn("Edep");
            analysisManager->CreateNtupleDColumn("energyInSoil");
            analysisManager->CreateNtupleIColumn("sourcePDG");
            analysisManager->CreateNtupleDColumn("zInSoil");
            analysisManager->CreateNtupleDColumn("rhoInSoil");
            analysisManager->FinishNtuple();
        }
    };
    std::visit(visitor, config);
}

RunAction::~RunAction() {}

// Saving data
void RunAction::BeginOfRunAction(const G4Run* aRun)
{
    startTime = std::chrono::steady_clock::now();
    std::visit(
        overloaded {
            [this](const CharacteriseSoilGammas& conf) {
                std::filesystem::create_directories(conf.outputDirectoryName);
                std::random_device rd;
                auto id = std::to_string(rd());

                if (conf.outputTypes == OutputTypes::INFO
                    || conf.outputTypes == OutputTypes::ALL) {
                    auto* analysisManager = G4AnalysisManager::Instance();
                    analysisManager->OpenFile(
                        conf.outputDirectoryName + "/out" + id
                    );
                }

                if (conf.outputTypes == OutputTypes::BINARY
                    || conf.outputTypes == OutputTypes::ALL) {
                    soilBinFile = std::ofstream(
                        conf.outputDirectoryName + "/out" + id + ".bin",
                        std::ios::app | std::ios::binary
                    );
                    soilBinEntries.clear();
                }
            },
            [this](const TransportGammas& conf) {
                transportExposure = 0.0;
                std::filesystem::create_directories(conf.outputDirectoryName);
                std::random_device rd;
                auto id = std::to_string(rd());

                if (conf.outputTypes == OutputTypes::INFO
                    || conf.outputTypes == OutputTypes::ALL) {
                    auto analysisManager = G4AnalysisManager::Instance();
                    analysisManager->OpenFile(
                        conf.outputDirectoryName + "/out" + id
                    );
                }

                if (conf.outputTypes == OutputTypes::BINARY
                    || conf.outputTypes == OutputTypes::ALL) {
                    preDetectorBinFile = std::ofstream(
                        conf.outputDirectoryName + "/out" + id + ".bin",
                        std::ios::app | std::ios::binary
                    );
                    preDetectorBinEntries.clear();
                }
            },
            [this](const Detect& conf) {
                detectionExposure = 0.0;
                std::filesystem::create_directories(conf.outputDirectoryName);
                std::random_device rd;
                auto id = std::to_string(rd());

                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->OpenFile(
                    conf.outputDirectoryName + "/out" + id
                );
            }
        },
        config
    );
}

void RunAction::EndOfRunAction(const G4Run* aRun)
{
    const auto endTime = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::duration<G4double>>(
            endTime - startTime
        );

    auto visitor = overloaded {
        [this](const CharacteriseSoilGammas& conf) {
            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                auto* analysisManager = G4AnalysisManager::Instance();
                analysisManager->Write();
                analysisManager->CloseFile();
            }

            const auto* runManager = G4RunManager::GetRunManager();
            const auto* generatorAction =
                (CRYPrimaryGeneratorAction*)
                    runManager->GetUserPrimaryGeneratorAction();

            const G4double exposure = generatorAction->GetExposureTime();

            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
                if (soilBinFile.is_open()) {
                    soilBinFile.write(
                        reinterpret_cast<const char*>(&exposure),
                        sizeof(exposure)
                    );
                    for (TransportGammaEntry entry : soilBinEntries) {
                        soilBinFile.write(
                            reinterpret_cast<const char*>(&entry.energy),
                            sizeof(entry.energy)
                        );
                        soilBinFile.write(
                            reinterpret_cast<const char*>(&entry.momentumZ),
                            sizeof(entry.momentumZ)
                        );
                        soilBinFile.write(
                            reinterpret_cast<const char*>(&entry.z),
                            sizeof(entry.z)
                        );
                        soilBinFile.write(
                            reinterpret_cast<const char*>(&entry.sourcePDG),
                            sizeof(entry.sourcePDG)
                        );
                    }
                    soilBinFile.close();
                }
            }

            G4cout << "Exposure: " << exposure << "s" << G4endl;
        },
        [this](const TransportGammas& conf) {
            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                auto* analysisManager = G4AnalysisManager::Instance();
                analysisManager->Write();
                analysisManager->CloseFile();
            }

            const auto* runManager = G4RunManager::GetRunManager();
            const auto* generatorAction =
                (TransportPrimaryGeneratorAction*)
                    runManager->GetUserPrimaryGeneratorAction();

            const G4double exposure = generatorAction->GetExposureTime();

            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
                if (preDetectorBinFile.is_open()) {
                    preDetectorBinFile.write(
                        reinterpret_cast<const char*>(&exposure),
                        sizeof(exposure)
                    );
                    for (PreDetectorEntry entry : preDetectorBinEntries) {
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.pdg),
                            sizeof(entry.pdg)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.z),
                            sizeof(entry.z)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.rho),
                            sizeof(entry.rho)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.energy),
                            sizeof(entry.energy)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.momentumZ),
                            sizeof(entry.momentumZ)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.momentumRho),
                            sizeof(entry.momentumRho)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(
                                &entry.momentumPhiDiff
                            ),
                            sizeof(entry.momentumPhiDiff)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.sourcePDG),
                            sizeof(entry.sourcePDG)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.energyInSoil),
                            sizeof(entry.energyInSoil)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.zInSoil),
                            sizeof(entry.zInSoil)
                        );
                        preDetectorBinFile.write(
                            reinterpret_cast<const char*>(&entry.rhoInSoil),
                            sizeof(entry.rhoInSoil)
                        );
                    }
                    preDetectorBinFile.close();
                }
            }
            G4cout << "Exposure: " << exposure << "s" << G4endl;
        },
        [this](const Detect& conf) {
            auto* analysisManager = G4AnalysisManager::Instance();
            const auto* runManager = G4RunManager::GetRunManager();
            const auto* generatorAction =
                (DetectionPrimaryGeneratorAction*)
                    runManager->GetUserPrimaryGeneratorAction();
            analysisManager->Write();
            analysisManager->CloseFile();

            const G4double exposure = generatorAction->GetExposureTime();
            G4cout << "Exposure: " << exposure << "s" << G4endl;
        }
    };

    std::visit(visitor, config);

    G4cout << "Run Finished. Elapsed: " << elapsed.count() << "s" << G4endl;
}

void RunAction::TallySoilGammaBinary(
    G4double energy,
    G4double momentumZ,
    G4double z,
    G4int sourcePDG
)
{
    TransportGammaEntry entry {energy, momentumZ, z, sourcePDG};
    soilBinEntries.push_back(entry);
}

void RunAction::TallyDetectorSurfaceBinary(
    G4int pdg,
    G4double z,
    G4double rho,
    G4double energy,
    G4double momentumZ,
    G4double momentumRho,
    G4double momentumPhiDiff,
    G4int sourcePDG,
    G4double energyInSoil,
    G4double zInSoil,
    G4double rhoInSoil
)
{
    PreDetectorEntry entry {
        pdg,
        z,
        rho,
        energy,
        momentumZ,
        momentumRho,
        momentumPhiDiff,
        sourcePDG,
        energyInSoil,
        zInSoil,
        rhoInSoil
    };
    preDetectorBinEntries.push_back(entry);
}

void RunAction::IncrementTransportExposure(G4double dt)
{
    transportExposure += dt;
}

G4double RunAction::GetTransportExposure() const
{
    return transportExposure;
}

void RunAction::IncrementDetectionExposure(G4double dt)
{
    detectionExposure += dt;
}

G4double RunAction::GetDetectionExposure() const
{
    return detectionExposure;
}