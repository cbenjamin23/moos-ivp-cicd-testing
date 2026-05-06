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

set(UFLDCOLLISIONDETECT_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uFldCollisionDetect)

add_moos_cpp_test(test_ufldcollisiondetect_cpa_monitor
  SOURCES
    CPAMonitorTest.cpp
    ${UFLDCOLLISIONDETECT_SRC}/CPAMonitor.cpp
  LIBS
    geodaid
    contacts
    geometry
    encounters
    mbutil
    ${MOOSGEODESY_LIBRARY}
    ${MOOS_LIBRARIES}
  INCLUDES
    ${UFLDCOLLISIONDETECT_SRC}
    ${MOOSGEODESY_INCLUDE_DIR}
  LABELS
    ufldcollisiondetect
    uFldCollisionDetect
    CollisionDetect
  SUITE_LABELS
    CPAMonitorTest=CPAMonitor
)
