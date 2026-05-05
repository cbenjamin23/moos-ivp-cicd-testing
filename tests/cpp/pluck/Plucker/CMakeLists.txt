set(PLUCK_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_pluck)

add_moos_cpp_test(test_pluck_plucker
  SOURCES
    PluckerTest.cpp
    ${PLUCK_SRC}/Plucker.cpp
  LIBS
    mbutil
  INCLUDES
    ${PLUCK_SRC}
  LABELS
    pluck
    Plucker
)
