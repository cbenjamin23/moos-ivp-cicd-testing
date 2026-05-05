set(ALOGTM_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogtm)

add_moos_cpp_test(test_alogtm_tgrep_handler
  SOURCES
    TGrepHandlerTest.cpp
    ${ALOGTM_SRC}/TGrepHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGTM_SRC}
  LABELS
    alogtm
    TGrepHandler
)
