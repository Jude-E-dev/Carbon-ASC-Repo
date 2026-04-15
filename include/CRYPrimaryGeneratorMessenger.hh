#pragma once

#include "G4UImessenger.hh"
#include "globals.hh"

class CRYPrimaryGeneratorAction;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithoutParameter;

class CRYPrimaryGeneratorMessenger: public G4UImessenger {
  public:
    CRYPrimaryGeneratorMessenger(CRYPrimaryGeneratorAction*);
    ~CRYPrimaryGeneratorMessenger();

    void SetNewValue(G4UIcommand*, G4String);

  private:
    CRYPrimaryGeneratorAction* Action;
    G4UIdirectory* CRYDir;
    G4UIcmdWithAString* FileCmd;
    G4UIcmdWithAString* InputCmd;
    G4UIcmdWithoutParameter* UpdateCmd;
    std::string* MessInput;
};