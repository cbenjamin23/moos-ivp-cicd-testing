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

add_moos_cpp_test(test_geodaid_contact_ledger
  SOURCES
    ContactLedgerTest.cpp
  LIBS
    geodaid
    contacts
    geometry
    mbutil
    ${MOOSGEODESY_LIBRARY}
    ${MOOS_LIBRARIES}
  INCLUDES
    ${MOOSGEODESY_INCLUDE_DIR}
  LABELS
    geodaid
    NodeReport
    pHelmIvP
    pContactMgrV20
    pMarineViewer
  SUITE_LABELS
    ContactLedgerTest=ContactLedger
    ColoredPointTest=ColoredPoint
)
