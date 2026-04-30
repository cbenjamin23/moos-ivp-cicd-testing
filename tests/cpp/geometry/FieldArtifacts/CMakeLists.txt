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

add_moos_cpp_test(test_geometry_field_artifacts
  SOURCES
    FieldArtifactsTest.cpp
  LIBS
    geometry
    mbutil
    ${MOOSGEODESY_LIBRARY}
  INCLUDES
    ${MOOSGEODESY_INCLUDE_DIR}
  LABELS
    geometry
    FieldArtifacts
    current
    artifacts
  SUITE_LABELS
    ArtifactUtilsLawnmowerTest=ArtifactUtils
    ArtifactUtilsArtifactsTest=ArtifactUtils
    CurrentFieldTest=CurrentField
    OpFieldTest=OpField
)
