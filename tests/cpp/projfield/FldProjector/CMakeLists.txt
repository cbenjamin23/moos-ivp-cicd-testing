set(PROJFIELD_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_projfield)

add_moos_cpp_test(test_projfield_fld_projector
  SOURCES
    FldProjectorTest.cpp
    ${PROJFIELD_SRC}/FldProjector.cpp
  LIBS
    geometry
    mbutil
  INCLUDES
    ${PROJFIELD_SRC}
  LABELS
    projfield
    FldProjector
)
