set(PECHOVAR_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/pEchoVar)

add_moos_cpp_test(test_pechovar_eflipper
  SOURCES
    EFlipperTest.cpp
    ${PECHOVAR_SRC}/EFlipper.cpp
  LIBS
    mbutil
  INCLUDES
    ${PECHOVAR_SRC}
  LABELS
    pechovar
    EFlipper
  SUITE_LABELS
    EFlipperTest=EFlipper
)
