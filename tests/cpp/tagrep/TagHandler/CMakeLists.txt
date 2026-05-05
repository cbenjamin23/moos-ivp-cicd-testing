set(TAGREP_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_tagrep)

add_moos_cpp_test(test_tagrep_tag_handler
  SOURCES
    TagHandlerTest.cpp
    ${TAGREP_SRC}/TagHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${TAGREP_SRC}
  LABELS
    tagrep
    TagHandler
)
