#include "ActionInitialization.hh"

#include "CRYPrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

ActionInitialization::ActionInitialization() : G4VUserActionInitialization() {}

ActionInitialization::~ActionInitialization() {}

void ActionInitialization::BuildForMaster() const {
    RunAction* runAction = new RunAction();
    SetUserAction(runAction);
}

void ActionInitialization::Build() const {
    CRYPrimaryGeneratorAction* primary =
        new CRYPrimaryGeneratorAction("setupCry.file");
    SetUserAction(primary);

    RunAction* runAction = new RunAction();

    SetUserAction(runAction);
    SetUserAction(new EventAction(runAction));
    SetUserAction(new SteppingAction());
}