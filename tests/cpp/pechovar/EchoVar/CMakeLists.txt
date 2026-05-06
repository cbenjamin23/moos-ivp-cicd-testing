set(PECHOVAR_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/pEchoVar)

add_moos_cpp_test(test_pechovar_echo_var
  SOURCES
    EchoVarTest.cpp
    ${PECHOVAR_SRC}/EchoVar.cpp
    ${PECHOVAR_SRC}/EFlipper.cpp
  LIBS
    apputil
    geometry
    logic
    mbutil
    ${MOOS_LIBRARIES}
  INCLUDES
    ${PECHOVAR_SRC}
  LABELS
    pechovar
    EchoVar
  SUITE_LABELS
    EchoVarMappingTest=EchoVar
    EchoVarFlipEntryTest=EchoVar
)
