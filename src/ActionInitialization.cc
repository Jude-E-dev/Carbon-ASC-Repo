#include "ActionInitialization.hh"

#include <G4SystemOfUnits.hh>
#include <G4Threading.hh>

#include "CRYPrimaryGeneratorAction.hh"
#include "Configs.hh"
#include "DetectionPrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"
#include "TransportPrimaryGeneratorAction.hh"
#include "overloaded.hh"

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
            SetUserAction(new CRYPrimaryGeneratorAction("setupCry.file"));
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