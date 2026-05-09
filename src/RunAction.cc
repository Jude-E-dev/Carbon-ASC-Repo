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

RunAction::RunAction(GCConfig config) :
    config {config},
    soilBinEntries(),
    soilBinFile(),
    detectorSurfaceBinEntries(),
    detectorSurfaceBinFile() {
    auto visitor = overloaded {
        [](const CharacteriseSoilGammas& conf) {
            if (conf.outputTypes == OutputTypes::BINARY)
                return;

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
        [](const TransportGammas& conf) {
            if (conf.outputTypes == OutputTypes::BINARY)
                return;

            auto analysisManager = G4AnalysisManager::Instance();
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
            auto analysisManager = G4AnalysisManager::Instance();
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
void RunAction::BeginOfRunAction(const G4Run* aRun) {
    startTime = std::chrono::steady_clock::now();
    std::visit(
        overloaded {
            [this](const CharacteriseSoilGammas& conf) {
                std::filesystem::create_directories(conf.outputDirectoryName);
                std::random_device rd;
                std::string id = std::to_string(rd());

                if (conf.outputTypes == OutputTypes::INFO
                    || conf.outputTypes == OutputTypes::ALL) {
                    auto analysisManager = G4AnalysisManager::Instance();
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
                std::string id = std::to_string(rd());

                if (conf.outputTypes == OutputTypes::INFO
                    || conf.outputTypes == OutputTypes::ALL) {
                    auto analysisManager = G4AnalysisManager::Instance();
                    analysisManager->OpenFile(
                        conf.outputDirectoryName + "/out" + id
                    );
                }

                if (conf.outputTypes == OutputTypes::BINARY
                    || conf.outputTypes == OutputTypes::ALL) {
                    detectorSurfaceBinFile = std::ofstream(
                        conf.outputDirectoryName + "/out" + id + ".bin",
                        std::ios::app | std::ios::binary
                    );
                    detectorSurfaceBinEntries.clear();
                }
            },
            [this](const Detect& conf) {
                detectionExposure = 0.0;
                std::filesystem::create_directories(conf.outputDirectoryName);
                std::random_device rd;
                std::string id = std::to_string(rd());

                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->OpenFile(
                    conf.outputDirectoryName + "/out" + id
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

    auto visitor = overloaded {
        [this](const CharacteriseSoilGammas& conf) {
            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                auto* analysisManager = G4AnalysisManager::Instance();
                analysisManager->Write();
                analysisManager->CloseFile();
            }

            auto* runManager = G4RunManager::GetRunManager();
            auto* generatorAction =
                (CRYPrimaryGeneratorAction*)
                    runManager->GetUserPrimaryGeneratorAction();

            double exposure = generatorAction->GetExposureTime();

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

            G4cout << "Exposure: " << exposure << G4endl;
        },
        [this](const TransportGammas& conf) {
            if (conf.outputTypes == OutputTypes::INFO
                || conf.outputTypes == OutputTypes::ALL) {
                auto analysisManager = G4AnalysisManager::Instance();
                analysisManager->Write();
                analysisManager->CloseFile();
            }

            auto* runManager = G4RunManager::GetRunManager();
            auto* generatorAction =
                (TransportPrimaryGeneratorAction*)
                    runManager->GetUserPrimaryGeneratorAction();

            double exposure = generatorAction->GetExposureTime();

            if (conf.outputTypes == OutputTypes::BINARY
                || conf.outputTypes == OutputTypes::ALL) {
                if (detectorSurfaceBinFile.is_open()) {
                    detectorSurfaceBinFile.write(
                        reinterpret_cast<const char*>(&exposure),
                        sizeof(exposure)
                    );
                    for (DetectorSurfaceEntry entry :
                         detectorSurfaceBinEntries) {
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.pdg),
                            sizeof(entry.pdg)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.z),
                            sizeof(entry.z)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.rho),
                            sizeof(entry.rho)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.energy),
                            sizeof(entry.energy)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.momentumZ),
                            sizeof(entry.momentumZ)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.momentumRho),
                            sizeof(entry.momentumRho)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(
                                &entry.momentumPhiDiff
                            ),
                            sizeof(entry.momentumPhiDiff)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.sourcePDG),
                            sizeof(entry.sourcePDG)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.energyInSoil),
                            sizeof(entry.energyInSoil)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.zInSoil),
                            sizeof(entry.zInSoil)
                        );
                        detectorSurfaceBinFile.write(
                            reinterpret_cast<const char*>(&entry.rhoInSoil),
                            sizeof(entry.rhoInSoil)
                        );
                    }
                    detectorSurfaceBinFile.close();
                }
            }
            G4cout << "Exposure: " << exposure << G4endl;
        },
        [this](const Detect& conf) {
            auto analysisManager = G4AnalysisManager::Instance();
            auto* runManager = G4RunManager::GetRunManager();
            auto* generatorAction =
                (DetectionPrimaryGeneratorAction*)
                    runManager->GetUserPrimaryGeneratorAction();
            analysisManager->Write();
            analysisManager->CloseFile();

            double exposure = generatorAction->GetExposureTime();
            G4cout << "Exposure: " << exposure << "s" << G4endl;
        }
    };

    std::visit(visitor, config);

    G4cout << "Run Finished. Elapsed: " << elapsed.count() << "s" << G4endl;
}

void RunAction::TallySoilGammaBinary(
    double energy,
    double momentumZ,
    double z,
    int sourcePDG
) {
    TransportGammaEntry entry;
    entry.energy = energy;
    entry.momentumZ = momentumZ;
    entry.z = z;
    entry.sourcePDG = sourcePDG;
    soilBinEntries.push_back(entry);
}

void RunAction::TallyDetectorSurfaceBinary(
    int pdg,
    double z,
    double rho,
    double energy,
    double momentumZ,
    double momentumRho,
    double momentumPhiDiff,
    int sourcePDG,
    double energyInSoil,
    double zInSoil,
    double rhoInSoil
) {
    DetectorSurfaceEntry entry;
    entry.pdg = pdg;
    entry.z = z;
    entry.rho = rho;
    entry.energy = energy;
    entry.momentumZ = momentumZ;
    entry.momentumRho = momentumRho;
    entry.momentumPhiDiff = momentumPhiDiff;
    entry.sourcePDG = sourcePDG;
    entry.energyInSoil = energyInSoil;
    entry.zInSoil = zInSoil;
    entry.rhoInSoil = rhoInSoil;
    detectorSurfaceBinEntries.push_back(entry);
}

void RunAction::IncrementTransportExposure(double dt) {
    transportExposure += dt;
}

double RunAction::GetTransportExposure() const {
    return transportExposure;
}

void RunAction::IncrementDetectionExposure(double dt) {
    detectionExposure += dt;
}

double RunAction::GetDetectionExposure() const {
    return detectionExposure;
}