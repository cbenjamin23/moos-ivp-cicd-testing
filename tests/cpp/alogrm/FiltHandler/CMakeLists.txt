set(ALOGRM_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogrm)

add_moos_cpp_test(test_alogrm_filt_handler
  SOURCES
    FiltHandlerTest.cpp
    ${ALOGRM_SRC}/FiltHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGRM_SRC}
  LABELS
    alogrm
    FiltHandler
)
