#pragma once

#include "G4UImessenger.hh"

class CRYPrimaryGeneratorAction;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithoutParameter;

class CRYPrimaryGeneratorMessenger: public G4UImessenger
{
  public:
    CRYPrimaryGeneratorMessenger(CRYPrimaryGeneratorAction*);

    CRYPrimaryGeneratorMessenger(const CRYPrimaryGeneratorMessenger&) = default;
    CRYPrimaryGeneratorMessenger(CRYPrimaryGeneratorMessenger&&) = delete;
    CRYPrimaryGeneratorMessenger&
    operator=(const CRYPrimaryGeneratorMessenger&) = default;
    CRYPrimaryGeneratorMessenger&
    operator=(CRYPrimaryGeneratorMessenger&&) = delete;
    ~CRYPrimaryGeneratorMessenger() override;

    void SetNewValue(G4UIcommand* /*command*/, G4String /*newValue*/) override;

  private:
    CRYPrimaryGeneratorAction* Action;
    G4UIdirectory* CRYDir;
    G4UIcmdWithAString* FileCmd;
    G4UIcmdWithAString* InputCmd;
    G4UIcmdWithoutParameter* UpdateCmd;
    G4String* MessInput;
};