#!/bin/sh
#
# Sets up the environment needed to configure, build, and run groundCarbon:
#   - Geant4 data paths (G4RADIOACTIVEDATA, G4LEDATA, etc.), via the install
#     this project is actually built against -- NOT any conda Geant4 env,
#     which may carry a different install and silently mismatch.
#   - CRYHOME, needed by CMake to locate the CRY library/headers.
#
# Usage: source env.sh   (must be sourced, not executed, so the exports
#                          persist in your shell)

_gc_env_dir="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" > /dev/null 2>&1 && pwd)"

_gc_g4_install="/Users/judeelzinga/Desktop/Geant4/install-11.4.2"

if [ -f "$_gc_g4_install/bin/geant4.sh" ]; then
  # shellcheck disable=SC1091
  . "$_gc_g4_install/bin/geant4.sh"
else
  echo "env.sh: WARNING: Geant4 install not found at $_gc_g4_install" >&2
  echo "         Update _gc_g4_install in this script if it has moved." >&2
fi

export CRYHOME="$(cd "$_gc_env_dir/../cry_v1.7" > /dev/null 2>&1 && pwd)"

if [ -z "$CRYHOME" ]; then
  echo "env.sh: WARNING: could not resolve cry_v1.7 relative to $_gc_env_dir" >&2
fi

unset _gc_env_dir _gc_g4_install
