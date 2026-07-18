find_package(MOOSGeodesy QUIET CONFIG
  HINTS
    ${MOOSIVP_SOURCE_TREE_BASE}/build/MOOS/MOOSGeodesy
)

if(TARGET MOOSGeodesy)
  set(MOOSGEODESY_TEST_LIBS MOOSGeodesy)
elseif(MOOSGeodesy_LIBRARIES)
  set(MOOSGEODESY_TEST_LIBS ${MOOSGeodesy_LIBRARIES})
else()
  find_library(MOOSGEODESY_LIBRARY
    NAMES MOOSGeodesy
    PATHS ${MOOSIVP_SOURCE_TREE_BASE}/build/MOOS/MOOSGeodesy/lib
    NO_DEFAULT_PATH
  )
  set(MOOSGEODESY_TEST_LIBS ${MOOSGEODESY_LIBRARY})
endif()

if(MOOSGeodesy_INCLUDE_DIRS)
  set(MOOSGEODESY_TEST_INCLUDES ${MOOSGeodesy_INCLUDE_DIRS})
else()
  find_path(MOOSGEODESY_INCLUDE_DIR
    NAMES MOOS/libMOOSGeodesy/MOOSGeodesy.h
    PATHS
      ${MOOSIVP_SOURCE_TREE_BASE}/MOOS/MOOSGeodesy/libMOOSGeodesy/include
      ${MOOSIVP_SOURCE_TREE_BASE}/MOOS_Jul2724/MOOSGeodesy/libMOOSGeodesy/include
      ${MOOSIVP_SOURCE_TREE_BASE}/MOOS_Jul0519/MOOSGeodesy/libMOOSGeodesy/include
    NO_DEFAULT_PATH
  )
  set(MOOSGEODESY_TEST_INCLUDES ${MOOSGEODESY_INCLUDE_DIR})
endif()

if(NOT MOOSGEODESY_TEST_LIBS OR NOT MOOSGEODESY_TEST_INCLUDES)
  message(FATAL_ERROR
    "Could not find the MOOSGeodesy library and headers under ${MOOSIVP_SOURCE_TREE_BASE}"
  )
endif()

add_moos_cpp_test(test_moosgeodesy_core
  SOURCES
    MOOSGeodesyTest.cpp
  LIBS
    ${MOOSGEODESY_TEST_LIBS}
  INCLUDES
    ${MOOSGEODESY_TEST_INCLUDES}
  LABELS
    moosgeodesy
    geodesy
    projection
  SUITE_LABELS
    MOOSGeodesyInitializationTest=Initialization
    MOOSGeodesyUTMTest=UTM
    MOOSGeodesyLocalGridTest=LocalGrid
    MOOSGeodesyMigrationContractTest=MigrationContract
    MOOSGeodesyValueSemanticsTest=ValueSemantics
    MOOSGeodesyUtilityTest=Utilities
)
