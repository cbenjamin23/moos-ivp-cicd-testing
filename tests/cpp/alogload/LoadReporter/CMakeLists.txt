set(ALOGLOAD_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogload)

add_moos_cpp_test(test_alogload_load_reporter
  SOURCES
    LoadReporterTest.cpp
    ${ALOGLOAD_SRC}/LoadReporter.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGLOAD_SRC}
  LABELS
    alogload
    LoadReporter
)
