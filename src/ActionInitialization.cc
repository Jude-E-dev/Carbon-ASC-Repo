#include "ActionInitialization.hh"

#include <G4SystemOfUnits.hh>

#include "CRYPrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "ReplayPrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"
#include "overloaded.hh"

ActionInitialization::ActionInitialization(GCConfig config) :
    G4VUserActionInitialization(),
    config {config} {}

ActionInitialization::~ActionInitialization() {}

void ActionInitialization::BuildForMaster() const {
    RunAction* runAction = new RunAction(config);
    SetUserAction(runAction);
}

void ActionInitialization::Build() const {
    auto* runAction = new RunAction(config);
    SetUserAction(runAction);
    SetUserAction(new EventAction(runAction, config));

    std::visit(
        overloaded {
            [this](const ReplayEmissions& conf) {
                SetUserAction(
                    new ReplayPrimaryGeneratorAction(conf.spectraPath, 50 * m)
                );
            },
            [this](auto const&) {
                SetUserAction(new CRYPrimaryGeneratorAction("setupCry.file"));
                SetUserAction(new SteppingAction(config));
            },
        },
        config
    );
}