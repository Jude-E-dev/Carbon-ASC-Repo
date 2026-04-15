#include "ActionInitialization.hh"

#include "CRYPrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "ReplayPrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

ActionInitialization::ActionInitialization(GCMode mode) :
    G4VUserActionInitialization(),
    mode {mode} {}

ActionInitialization::~ActionInitialization() {}

void ActionInitialization::BuildForMaster() const {
    RunAction* runAction = new RunAction(mode);
    SetUserAction(runAction);
}

void ActionInitialization::Build() const {
    auto* runAction = new RunAction(mode);
    SetUserAction(runAction);
    SetUserAction(new EventAction(runAction, mode));

    switch (mode) {
        case GCMode::CharacteriseSoil:
            SetUserAction(new CRYPrimaryGeneratorAction("setupCry.file"));
            SetUserAction(new SteppingAction);
            break;
        case GCMode::ReplayEmissions:
            SetUserAction(new ReplayPrimaryGeneratorAction());
            break;
        default:
            break;
    }
}