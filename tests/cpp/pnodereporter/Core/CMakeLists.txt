set(PNODEREPORTER_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/pNodeReporter)

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

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NodeReporterTest.cpp")
  message(STATUS "Skipping test_pnodereporter_core: missing NodeReporterTest.cpp")
  return()
endif()

add_moos_cpp_test(test_pnodereporter_core
  SOURCES
    NodeReporterTest.cpp
    ${PNODEREPORTER_SRC}/NodeReporter.cpp
  LIBS
    logutils
    contacts
    apputil
    geometry
    mbutil
    ${MOOS_LIBRARIES}
    ${MOOSGEODESY_LIBRARY}
  INCLUDES
    ${PNODEREPORTER_SRC}
    ${MOOSGEODESY_INCLUDE_DIR}
  LABELS
    pnodereporter
    pNodeReporter
  SUITE_LABELS
    NodeReporterTest=NodeReporter
)
