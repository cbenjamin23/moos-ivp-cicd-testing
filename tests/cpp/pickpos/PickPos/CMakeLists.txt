set(PICKPOS_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_pickpos)

add_moos_cpp_test(test_pickpos_pick_pos
  SOURCES
    PickPosTest.cpp
    ${PICKPOS_SRC}/PickPos.cpp
  LIBS
    geometry
    mbutil
  INCLUDES
    ${PICKPOS_SRC}
  LABELS
    pickpos
    PickPos
)
