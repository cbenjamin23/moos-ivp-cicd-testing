set(ALOGAVG_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogavg)

add_moos_cpp_test(test_alogavg_avg_handler
  SOURCES
    AvgHandlerTest.cpp
    ${ALOGAVG_SRC}/AvgHandler.cpp
    ${ALOGAVG_SRC}/ExpEntry.cpp
  LIBS
    apputil
    logutils
    mbutil
  INCLUDES
    ${ALOGAVG_SRC}
  LABELS
    alogavg
    AvgHandler
)
