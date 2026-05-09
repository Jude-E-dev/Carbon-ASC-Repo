
#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.16...3.27)
project(ground-carbon)

set(useROOT false)
#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
include(${Geant4_USE_FILE})
#Setup CRY-----------------------------------
find_library(CRY NAMES CRY PATHS $ENV{CRYHOME}/lib NO_DEFAULT_PATH)

set(CRY_INCLUDE_DIRS $ENV{CRYHOME}/src)
MESSAGE("Cry setup: " ${CRY_INCLUDE_DIRS})
set(CRY_LIBRARIES $ENV{CRYHOME}/lib/libCRY.a)

include_directories(${PROJECT_SOURCE_DIR}/include
  ${CRY_INCLUDE_DIRS}
)
if(useROOT)
  find_package(ROOT)
  EXECUTE_PROCESS(COMMAND root-config --cflags OUTPUT_VARIABLE ROOT_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
endif(useROOT)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(groundCarbon groundCarbon.cc ${sources} ${headers})
if(useROOT)
  EXECUTE_PROCESS(COMMAND root-config --libs OUTPUT_VARIABLE ROOT_LD_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_EXE_LINKER_FLAGS ${ROOT_LD_FLAGS})
  set(ROOT_LIBRARIES ${ROOT_LIBRARIES})
endif(useROOT)
target_link_libraries(groundCarbon ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} ${CRY_LIBRARIES})

#----------------------------------------------------------------------------
# Include root
#find_package(ROOT)
#include_directories(${ROOT_INCLUDE_DIR})
#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build B1. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(GROUNDCARBON_SCRIPTS
  macros/init_vis.mac
  macros/vis.mac
  setupCry.file
)

foreach(_script ${GROUNDCARBON_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
  )
endforeach()

#----------------------------------------------------------------------------
# For internal Geant4 use - but has no effect if you build this
# example standalone
#
add_custom_target(B1 DEPENDS groundCarbon)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS groundCarbon DESTINATION bin)

#-------------------------------- For clangd --------------------------------
#
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
