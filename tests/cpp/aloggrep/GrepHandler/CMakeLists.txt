set(ALOGGREP_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_aloggrep)

add_moos_cpp_test(test_aloggrep_grep_handler
  SOURCES
    GrepHandlerTest.cpp
    ${ALOGGREP_SRC}/GrepHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGGREP_SRC}
  LABELS
    aloggrep
    GrepHandler
)
