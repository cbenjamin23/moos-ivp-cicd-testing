set(ALOGTEST_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogtest)

add_moos_cpp_test(test_alogtest_log_tester
  SOURCES
    LogTesterTest.cpp
    ${ALOGTEST_SRC}/LogTest.cpp
    ${ALOGTEST_SRC}/LogTester.cpp
  LIBS
    logic
    logutils
    mbutil
  INCLUDES
    ${ALOGTEST_SRC}
  LABELS
    alogtest
    LogTest
    LogTester
)
