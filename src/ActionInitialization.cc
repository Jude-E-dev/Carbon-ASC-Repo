#include "ActionInitialization.hh"

#include <G4SystemOfUnits.hh>
#include <G4Threading.hh>

#include "CRYPrimaryGeneratorAction.hh"
#include "Configs.hh"
#include "DetectionPrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "NORMPrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"
#include "TransportPrimaryGeneratorAction.hh"
#include "overloaded.hh"

namespace
{
#ifndef GC_CRY_DATA_PATH
#error \
    "GC_CRY_DATA_PATH is not defined -- CRYHOME must be set in the environment when running CMake (see env.sh)"
#endif
// Baked in at configure time from $CRYHOME, the same env var CMakeLists.txt
// already uses to locate the CRY library/headers -- see env.sh. Do not
// hardcode a developer-specific path here; it won't exist on other machines.
constexpr std::string_view CRY_DATA_PATH {GC_CRY_DATA_PATH};
constexpr std::string_view CRY_SETUP_PATH {"setupCry.file"};
} // namespace

ActionInitialization::ActionInitialization(
    const SoilConfig& soil,
    const GCConfig& config,
    int updatePeriod
)
    : config {config}
    , soil {soil}
    , updatePeriod {updatePeriod}
{
}

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction(config));
}

void ActionInitialization::Build() const
{
    auto* runAction = new RunAction(config);
    auto* eventAction = new EventAction(config, updatePeriod);
    SetUserAction(runAction);
    SetUserAction(eventAction);
    SetUserAction(new SteppingAction(config, eventAction, runAction));

    auto visitor = overloaded {
        [&, this](const CharacteriseSoilGammas& conf) {
            if (conf.source == PrimarySource::Cosmogenic) {
                SetUserAction(new CRYPrimaryGeneratorAction(
                    CRY_SETUP_PATH,
                    CRY_DATA_PATH
                ));
            }
            else {
                SetUserAction(new NORMPrimaryGeneratorAction(soil));
            }
            SetUserAction(new TrackingAction(eventAction));
        },
        [&, this](const TransportGammas& conf) {
            SetUserAction(new TransportPrimaryGeneratorAction(
                conf.spectraPath,
                soil,
                eventAction,
                runAction
            ));
        },
        [&, this](const Detect& conf) {
            SetUserAction(new DetectionPrimaryGeneratorAction(
                conf.spectraPath,
                eventAction,
                runAction
            ));
        },
    };

    std::visit(visitor, config);
}