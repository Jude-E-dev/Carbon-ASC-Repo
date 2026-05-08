#pragma once

#include <G4Track.hh>
#include <G4UserEventAction.hh>
#include <string>
#include <unordered_map>

#include "Configs.hh"

class RunAction;
class G4Step;

class EventAction: public G4UserEventAction {
  public:
    EventAction(RunAction* runAction, GCConfig config, int updatePeriod);
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    void RegisterPrimary(int trackID, std::string primaryName, int pdg);
    std::string GetPrimaryName(const G4Track* track);
    int GetPrimaryPDG(const G4Track* track);

    void SetSourceProperties(double depth, double radius, double energy, int pdg);
    double GetSourceDepth();
    double GetSourceRadius();
    double GetSourceEnergy();
    int GetSourcePDG();
    void DepositEnergy(double eDep);
    double GetPulseEnergy();

  private:
    RunAction* fRunAction;
    GCConfig config;
    int updatePeriod;
    std::unordered_map<int, std::string> primariesMap;
    std::unordered_map<int, int> pdgsMap;

    // Only used in GCConfig::Replay
    double sourceDepth = 0.0;
    double sourceRadius = 0.0;
    double sourceEnergy = 0.0;
    int sourcePDG = 0;
    double pulseEnergy = 0.0;
};