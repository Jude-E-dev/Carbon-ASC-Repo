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
