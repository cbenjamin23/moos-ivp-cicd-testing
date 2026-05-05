set(ALOGPARE_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogpare)

add_moos_cpp_test(test_alogpare_pare_engine
  SOURCES
    PareEngineTest.cpp
    ${ALOGPARE_SRC}/PareEngine.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGPARE_SRC}
  LABELS
    alogpare
    PareEngine
)
