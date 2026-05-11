set(USIM_MARINE_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uSimMarineV22)

find_library(MOOSGEODESY_LIBRARY
  NAMES MOOSGeodesy
  PATHS ${MOOSIVP_SOURCE_TREE_BASE}/build/MOOS/MOOSGeodesy/lib
  NO_DEFAULT_PATH
)

find_path(MOOSGEODESY_INCLUDE_DIR
  NAMES MOOS/libMOOSGeodesy/MOOSGeodesy.h
  PATHS ${MOOSIVP_SOURCE_TREE_BASE}/MOOS/MOOSGeodesy/libMOOSGeodesy/include
  NO_DEFAULT_PATH
)

if(NOT MOOSGEODESY_LIBRARY OR NOT MOOSGEODESY_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find MOOSGeodesy library/include under ${MOOSIVP_SOURCE_TREE_BASE}")
endif()

foreach(required_source
    ThrustMapTest.cpp
    TurnSpeedMapTest.cpp
    SimEngineTest.cpp
    WormHoleTest.cpp
    USMModelTest.cpp
    USMMOOSAppTest.cpp)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${required_source}")
    message(STATUS "Skipping test_usim_marine_core: missing ${required_source}")
    return()
  endif()
endforeach()

add_moos_cpp_test(test_usim_marine_core
  SOURCES
    ThrustMapTest.cpp
    TurnSpeedMapTest.cpp
    SimEngineTest.cpp
    WormHoleTest.cpp
    USMModelTest.cpp
    USMMOOSAppTest.cpp
    ${USIM_MARINE_SRC}/ThrustMap.cpp
    ${USIM_MARINE_SRC}/TurnSpeedMap.cpp
    ${USIM_MARINE_SRC}/SimEngine.cpp
    ${USIM_MARINE_SRC}/WormHole.cpp
    ${USIM_MARINE_SRC}/WormHoleSet.cpp
    ${USIM_MARINE_SRC}/USM_Model.cpp
    ${USIM_MARINE_SRC}/USM_MOOSApp.cpp
  LIBS
    polar
    marine_pid
    contacts
    geometry
    apputil
    mbutil
    ${MOOS_LIBRARIES}
    ${MOOSGEODESY_LIBRARY}
  INCLUDES
    ${USIM_MARINE_SRC}
    ${MOOSGEODESY_INCLUDE_DIR}
  LABELS
    usim_marine
    usim-marine
    uSimMarineV22
  SUITE_LABELS
    ThrustMapTest=ThrustMap
    TurnSpeedMapTest=TurnSpeedMap
    SimEngineTest=SimEngine
    WormHoleTest=WormHole
    WormHoleSetTest=WormHoleSet
    USMModelTest=USM_Model
    USMMOOSAppTest=USM_MOOSApp
)
