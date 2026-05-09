#pragma once

#include <G4String.hh>
#include <G4Track.hh>
#include <G4UserEventAction.hh>
#include <unordered_map>

#include "Configs.hh"

class RunAction;
class G4Step;

/**
 * @brief Code that runs per-particle-event
 * 
 */
class EventAction: public G4UserEventAction
{
  public:
    /**
   * @brief Construct a new Event Action object
   * 
   * @param config Configuration information for the whole run
   * @param updatePeriod Print update information every `updatePeriod` events
   */
    EventAction(const GCConfig& config, G4int updatePeriod);

    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    /**
     * @brief Register the name and PDG of the cosmic ray primary that the
     * given track came from
     * 
     * @param trackID The track ID to register this primary information to
     * @param primaryName The name of the original cosmic ray primary that led
     * to this track
     * @param pdg The PDG of the original cosmic ray primary that led to this
     * track
     */
    void RegisterPrimary(G4int trackID, const G4String& primaryName, G4int pdg);

    /**
     * @brief Get the name of the original cosmic ray primary that led to the
     * given track
     * 
     * @param track The track to query
     * @return G4String The cosmic ray primary. If no primary is registered to
     * this track, an error string will be returned
     */
    G4String GetPrimaryName(const G4Track* track) const;

    /**
     * @brief Get the PDG of the original cosmic ray primary that led to the
     * given track
     * 
     * @param track The track to query
     * @return G4int The cosmic ray primary. If no primary is registered to
     * this track, -1 will be returned
     */
    G4int GetPrimaryPDG(const G4Track* track) const;

    /**
     * @brief Store information about the gamma ray being replayed in this
     * transport run.
     * 
     * @param z The depth of the gamma ray in the soil when it was produced
     * @param rho The distance from the z-axis of the gamma ray when it was
     * produced
     * @param energy The energy of the gamma ray when it was produced
     * @param pdg The PDG of the cosmic ray particle that led to this gamma
     * ray being produced
     */
    void SetSoilSourceProperties(
        G4double z,
        G4double rho,
        G4double energy,
        G4int pdg
    );

    /**
     * @brief Get the depth of the gamma ray that was replayed in this event,
     * when it was produced
     * 
     * @return G4double Depth of the gamma ray at production
     */
    G4double GetSoilSourceZ() const;

    /**
     * @brief Get the distance from the z-axis of the gamma ray that was
     * replayed in this event, when it was produced
     * 
     * @return G4double Distance from z-axis of the gamma ray at production
     */
    G4double GetSoilSourceRho() const;

    /**
     * @brief Get the energy of the gamma ray that was replayed in this event,
     * when it was produced
     * 
     * @return G4double Energy of the gamma ray at production
     */
    G4double GetSoilSourceEnergy() const;
    /**
     * @brief Get the PDG of the cosmic ray that created the gamma being
     * replayed in this event
     * 
     * @return G4int PDG of the original cosmic ray particle
     */
    G4int GetSoilSourcePDG() const;

    /**
     * @brief Store information about the gamma ray that led to the current
     * track being replayed into the detector
     * 
     * @param energyInSoil The energy of the gamma ray that created this event,
     * when it was produced in the soil
     * @param sourcePDG The PDG of the cosmic ray that led to the production of
     * the gamma ray leading to this track
     * @param zInSoil The depth of the gamma ray that created this event, when
     * it was produced in the soil
     * @param rhoInSoil The distance from the z-axis of the gamma ray that
     * created this event, when it was produced in the soil
     */
    void SetDetectionProperties(
        G4double energyInSoil,
        G4int sourcePDG,
        G4double zInSoil,
        G4double rhoInSoil
    );

    /**
     * @brief Get the energy of the gamma ray (at production) that led to the
     * track currently being simulated
     * 
     * @return G4double The gamma ray energy when it was produced in the soil
     */
    G4double GetDetectionStartingEnergyInSoil() const;

    /**
     * @brief Get the PDG of the cosmic ray particle that led to the gamma ray
     * which created the track being simulated
     * 
     * @return G4int The PDG of the cosmic ray that (indirectly) created this
     * track
     */
    G4int GetDetectionSourcePDG() const;

    /**
     * @brief Get the depth of the gamma ray that led to the track being
     * simulated, when it was produced in the soil
     * 
     * @return G4double The depth of the gamma ray at production in the soil
     */
    G4double GetDetectionSoilZ() const;
    
    /**
     * @brief Get the distance from the z-axis of the gamma ray that led to the
     * track being simulated, when it was produced in the soil
     * 
     * @return G4double The distance from the z-axis of the gamma ray at
     * production in the soil
     */
    G4double GetDetectionSoilRho() const;

    /**
     * @brief Deposit energy in the detector during a 'detect' run. This is
     * accumulated across an event
     * 
     * @param eDep The energy to deposit
     */
    void DepositEnergy(G4double eDep);

    /**
     * @brief Get the energy of the detector pulse from the current event
     * 
     * @return G4double The energy of the detector pulse
     */
    G4double GetDetectionPulseEnergy() const;

  private:
    GCConfig config;
    G4int updatePeriod;
    std::unordered_map<G4int, G4String> primariesMap;
    std::unordered_map<G4int, G4int> pdgsMap;

    // Only used in GCConfig::Transport
    G4double soilSourceZ = 0.0;
    G4double soilSourceRho = 0.0;
    G4double soilSourceEnergy = 0.0;
    G4int soilSourcePDG = 0;

    // Only used in GCConfig::Detect
    G4double detectionEnergyInSoil = 0.0;
    G4int detectionSourcePDG = 0;
    G4double detectionZInSoil = 0.0;
    G4double detectionRhoInSoil = 0.0;
    G4double detectionPulseEnergy = 0.0;
};