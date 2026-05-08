#include "ActionInitialization.hh"

#include <G4SystemOfUnits.hh>

#include "CRYPrimaryGeneratorAction.hh"
#include "Configs.hh"
#include "EventAction.hh"
#include "ReplayPrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"
#include "overloaded.hh"

ActionInitialization::ActionInitialization(
    SoilConfig soil,
    GCConfig config,
    int updatePeriod
) :
    G4VUserActionInitialization(),
    config {config},
    soil {soil},
    updatePeriod {updatePeriod} {}

ActionInitialization::~ActionInitialization() {}

void ActionInitialization::BuildForMaster() const {
    RunAction* runAction = new RunAction(config);
    SetUserAction(runAction);
}

void ActionInitialization::Build() const {
    auto* runAction = new RunAction(config);
    auto* eventAction = new EventAction(runAction, config, updatePeriod);
    SetUserAction(runAction);
    SetUserAction(eventAction);

    

    std::visit(
        overloaded {
            [this](const ReplayEmissions& conf) {
                SetUserAction(
                    new ReplayPrimaryGeneratorAction(conf.spectraPath, soil)
                );
            },
            [this, eventAction](auto const&) {
                SetUserAction(new CRYPrimaryGeneratorAction("setupCry.file"));
                SetUserAction(new SteppingAction(config, eventAction));
                SetUserAction(new TrackingAction(eventAction));
            },
        },
        config
    );
}