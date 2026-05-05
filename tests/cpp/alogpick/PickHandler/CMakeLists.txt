set(ALOGPICK_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogpick)

add_moos_cpp_test(test_alogpick_pick_handler
  SOURCES
    PickHandlerTest.cpp
    ${ALOGPICK_SRC}/PickHandler.cpp
  LIBS
    apputil
    logutils
    mbutil
  INCLUDES
    ${ALOGPICK_SRC}
  LABELS
    alogpick
    PickHandler
)
