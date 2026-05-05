set(ALOGEPLOT_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogeplot)

add_moos_cpp_test(test_alogeplot_eplot_engine
  SOURCES
    EPlotEngineTest.cpp
    ${ALOGEPLOT_SRC}/EPlotEngine.cpp
  LIBS
    encounters
    logutils
    mbutil
  INCLUDES
    ${ALOGEPLOT_SRC}
  LABELS
    alogeplot
    EPlotEngine
)
