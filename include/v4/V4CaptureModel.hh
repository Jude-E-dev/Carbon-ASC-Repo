#pragma once

#include <G4HadFinalState.hh>
#include <G4HadronicInteraction.hh>

class G4NeutronHPCapture;

/**
 * @brief Neutron capture below 20 MeV: evaluated-data capture photons
 * (G4NeutronHPCapture) for every target except hydrogen-1, which gets exact
 * two-body kinematics, n + p -> d + gamma (one 2.2232 MeV photon at thermal
 * energies).
 *
 * Why: G4NDL has no capture photon data for H-1, so G4NeutronHPCapture falls
 * back to G4PhotonEvaporation, which on Geant4 11.4 turns the 2.2246 MeV
 * deuteron excitation into a multi-photon continuum and the 2.223 MeV line
 * all but disappears. G4NeutronRadCaptureHP (11.4's default) gets hydrogen
 * right but drops the evaluated Si-28 and Fe-56 lines. See PREP_NOTES_V4.md.
 *
 * Known issue (found after the v4 campaign; left as is so this code matches
 * the published results): G4NeutronHPCapture re-selects the target element
 * from the material itself. A capture the process assigned to a non-hydrogen
 * nucleus can therefore still land on H-1 inside `hp` (with probability
 * about the hydrogen share of captures, ~0.37 in the campaign soils) and get
 * HP's own H-1 photons. The 2.223 MeV line comes out too strong (about x1.5)
 * and the other capture lines too weak (x(1 - p_H)); the v4 analysis
 * corrects both per run. Fix: always call `hp`, and replace its final state
 * with the exact n + p result only when `hp` itself chose H-1.
 */
class V4CaptureModel: public G4HadronicInteraction
{
  public:
    V4CaptureModel();
    ~V4CaptureModel() override;

    G4HadFinalState* ApplyYourself(const G4HadProjectile& projectile,
                                   G4Nucleus& target) override;
    void BuildPhysicsTable(const G4ParticleDefinition& particle) override;
    void InitialiseModel() override;
    const std::pair<G4double, G4double> GetFatalEnergyCheckLevels() const override;

  private:
    G4NeutronHPCapture* hp;
    G4HadFinalState hydrogenResult;
    G4int secondaryModelID;
};
