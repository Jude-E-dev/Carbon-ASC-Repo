#pragma once

#include <G4VModularPhysicsList.hh>

#include <string>

/**
 * @brief The project physics list (GCPhysicsList: EM option 4, EM extra,
 * decay, radioactive decay, HP elastic, stopping, ions) with the hadronic
 * inelastic set selectable (FTFP_BERT_HP or QGSP_BIC_HP) and, optionally,
 * the thermal scattering law for hydrogen in water (G4ThermalNeutrons).
 *
 * Neutron capture final state: from Geant4 11.4 the HP lists use
 * G4NeutronRadCaptureHP (PhotonEvaporation cascades), which drops the
 * dominant Si-28(n,g) lines at 3.539 and 4.934 MeV. Geant4 11.3 and earlier
 * (the v1-v3 campaigns) use G4NeutronHPCapture, which samples the evaluated
 * (G4NDL) capture photons where they exist. captureModel "hp" (default)
 * installs V4CaptureModel below 20 MeV on every version (G4NeutronHPCapture,
 * with exact kinematics for H-1, which has no G4NDL photon data), so results
 * do not depend on the installed release; "default" keeps the list's own.
 */
class V4PhysicsList: public G4VModularPhysicsList
{
  public:
    V4PhysicsList(const std::string& hadronic, bool thermalScattering,
                  const std::string& captureModel);
    void ConstructProcess() override;

  private:
    std::string captureModel;
};
