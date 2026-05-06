set(PREALM_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/pRealm)

add_moos_cpp_test(test_prealm_pipeway
  SOURCES
    PipeWayTest.cpp
    ${PREALM_SRC}/PipeWay.cpp
  LIBS
    mbutil
  INCLUDES
    ${PREALM_SRC}
  LABELS
    prealm
    PipeWay
  SUITE_LABELS
    PipeWayTest=PipeWay
)
