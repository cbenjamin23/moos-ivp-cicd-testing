set(ALOGCLIP_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogclip)

add_moos_cpp_test(test_alogclip_alogclip
  SOURCES
    ALogClipTest.cpp
    ${ALOGCLIP_SRC}/ALogClipper.cpp
    ${ALOGCLIP_SRC}/ALogClipHandler.cpp
  LIBS
    mbutil
  INCLUDES
    ${ALOGCLIP_SRC}
  LABELS
    alogclip
    ALogClip
)
