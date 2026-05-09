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
    SetUserAction(runAction);
    SetUserAction(new EventAction(runAction, config, updatePeriod));
    SetUserAction(new SteppingAction(config));

    auto visitor = overloaded {
        [this](const CharacteriseSoilGammas& conf) {
            SetUserAction(new CRYPrimaryGeneratorAction("setupCry.file"));
            SetUserAction(new TrackingAction());
        },
        [this](const TransportGammas& conf) {
            SetUserAction(
                new TransportPrimaryGeneratorAction(conf.spectraPath, soil)
            );
        },
        [this](const Detect& conf) {
            SetUserAction(
                new DetectionPrimaryGeneratorAction(conf.spectraPath)
            );
        },
    };

    std::visit(visitor, config);
}