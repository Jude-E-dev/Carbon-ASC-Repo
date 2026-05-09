#include "RNGWrapper.hh"

#include <CLHEP/Random/RandomEngine.h>

#include <G4Types.hh>

template CLHEP::HepRandomEngine* RNGWrapper<CLHEP::HepRandomEngine>::m_obj;

template G4double (CLHEP::HepRandomEngine::* RNGWrapper<CLHEP::HepRandomEngine>::m_func)(
    void
);
