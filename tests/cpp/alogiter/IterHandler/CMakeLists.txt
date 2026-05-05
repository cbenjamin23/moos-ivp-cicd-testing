set(ALOGITER_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogiter)

add_moos_cpp_test(test_alogiter_iter_handler
  SOURCES
    IterHandlerTest.cpp
    ${ALOGITER_SRC}/IterHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGITER_SRC}
  LABELS
    alogiter
    IterHandler
)
