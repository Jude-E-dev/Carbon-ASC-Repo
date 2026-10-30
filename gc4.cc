/**
 * @file gc4.cc
 * @brief v4 driver for the ground-carbon simulation (see campaign_v4/README_V4.md).
 *
 * One binary for every soil: compositions are read at run time from `.soil`
 * files. Modes:
 *   produce       CRY (or test-beam) primaries into a laterally large soil;
 *                 every gamma born in the soil is recorded with its origin
 *                 (process, target nucleus, parent) and killed; neutron
 *                 track-length tallies.
 *   norm-library  K-40 / U-238 / Th-232 decay chains at rest; records the
 *                 emitted gammas (energies, times) per chain.
 *   transport     replays production records or samples the NORM library,
 *                 with importance-sampled lateral positions, through soil and
 *                 air into the germanium detector; weighted Edep histograms.
 *   analog        CRY above soil and detector, everything tracked, detector
 *                 pulses with times and origin, tag-volume crossings.
 *
 * The legacy three-stage `groundCarbon` binary (v1-v3) is built separately.
 */

#include <G4RunManagerFactory.hh>
#include <G4UImanager.hh>
#include <G4ios.hh>
#include <G4ParticleHPManager.hh>
#include <Randomize.hh>

#include <exception>
#include <string>
#include <vector>

#include "v4/SoilModel.hh"
#include "v4/V4Actions.hh"
#include "v4/V4DetectorConstruction.hh"
#include "v4/V4Options.hh"
#include "v4/V4PhysicsList.hh"

int main(int argc, char* argv[])
{
    const std::vector<std::string> args(argv, argv + argc);
    if (args.size() >= 2 && (args[1] == "help" || args[1] == "--help")) {
        G4cout << V4Options::Usage() << G4endl;
        return 0;
    }

    try {
        const V4Options options = V4Options::Parse(args);
        const SoilModel soil = SoilModel::Load(options.soilPath);

        CLHEP::HepRandom::setTheEngine(new CLHEP::MixMaxRng);
        CLHEP::HepRandom::setTheSeed(static_cast<long>(options.seed));

        if (options.hpPhotoEvaporation) {
            G4ParticleHPManager::GetInstance()->SetUseOnlyPhotoEvaporation(true);
        }

        auto* runManager =
            G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
        runManager->SetUserInitialization(new V4DetectorConstruction(soil, options));
        runManager->SetUserInitialization(
            new V4PhysicsList(options.physics, options.thermalScattering, options.captureModel)
        );
        auto* shared = new V4Shared(options, soil);
        runManager->SetUserInitialization(new V4ActionInitialization(shared));

        auto* ui = G4UImanager::GetUIpointer();
        ui->ApplyCommand("/run/initialize");
        // K-40, U-238 and Th-232 have Gyr half-lives; without this they would
        // be treated as stable (as in the legacy binary).
        ui->ApplyCommand("/process/had/rdm/thresholdForVeryLongDecayTime 1.0e11 year");

        std::uint64_t events = options.events;
        const auto* generator = dynamic_cast<const V4Generator*>(
            runManager->GetUserPrimaryGeneratorAction()
        );
        if (generator != nullptr && generator->PlannedEvents() > 0) {
            events = generator->PlannedEvents();
        }
        if (events == 0) {
            throw std::runtime_error("nothing to do: 0 events planned");
        }
        G4cout << "gc4: " << options.Describe() << " -> " << events << " events" << G4endl;
        runManager->BeamOn(static_cast<G4int>(events));

        delete runManager;
        delete shared;
    }
    catch (const std::exception& error) {
        G4cerr << "gc4: " << error.what() << G4endl;
        return 1;
    }
    return 0;
}
