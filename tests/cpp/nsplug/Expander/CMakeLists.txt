set(NSPLUG_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_nsplug)

add_moos_cpp_test(test_nsplug_expander
  SOURCES
    ExpanderTest.cpp
    ${NSPLUG_SRC}/Expander.cpp
  LIBS
    mbutil
  INCLUDES
    ${NSPLUG_SRC}
  LABELS
    nsplug
    Expander
)
