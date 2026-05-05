set(ALOGCAT_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogcat)

add_moos_cpp_test(test_alogcat_alog_cat_handler
  SOURCES
    ALogCatHandlerTest.cpp
    ${ALOGCAT_SRC}/ALogCatHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGCAT_SRC}
  LABELS
    alogcat
    ALogCatHandler
)
