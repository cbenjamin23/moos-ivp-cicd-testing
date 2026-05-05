set(GEN_MOOS_APP_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_gen_moos_app)

add_moos_cpp_test(test_gen_moos_app_app_generator
  SOURCES
    AppGeneratorTest.cpp
    ${GEN_MOOS_APP_SRC}/AppGenerator.cpp
  INCLUDES
    ${GEN_MOOS_APP_SRC}
  LABELS
    gen_moos_app
    AppGenerator
)
