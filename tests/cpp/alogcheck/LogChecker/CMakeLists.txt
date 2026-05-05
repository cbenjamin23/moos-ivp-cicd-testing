set(ALOGCHECK_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogcheck)

add_moos_cpp_test(test_alogcheck_log_checker
  SOURCES
    LogCheckerTest.cpp
    ${ALOGCHECK_SRC}/LogChecker.cpp
  LIBS
    logic
    logutils
    mbutil
  INCLUDES
    ${ALOGCHECK_SRC}
  LABELS
    alogcheck
    LogChecker
)
