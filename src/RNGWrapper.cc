#include "RNGWrapper.hh"

#include <CLHEP/Random/RandomEngine.h>

template CLHEP::HepRandomEngine* RNGWrapper<CLHEP::HepRandomEngine>::m_obj;

template double (CLHEP::HepRandomEngine::*
                     RNGWrapper<CLHEP::HepRandomEngine>::m_func)(void);
