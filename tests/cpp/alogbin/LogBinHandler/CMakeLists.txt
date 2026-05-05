set(ALOGBIN_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogbin)

add_moos_cpp_test(test_alogbin_log_bin_handler
  SOURCES
    LogBinHandlerTest.cpp
    ${ALOGBIN_SRC}/LogBinHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGBIN_SRC}
  LABELS
    alogbin
    LogBinHandler
)
