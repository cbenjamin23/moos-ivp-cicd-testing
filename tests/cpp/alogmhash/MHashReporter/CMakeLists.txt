set(ALOGMHASH_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogmhash)

add_moos_cpp_test(test_alogmhash_mhash_reporter
  SOURCES
    MHashReporterTest.cpp
    ${ALOGMHASH_SRC}/MHashReporter.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGMHASH_SRC}
  LABELS
    alogmhash
    MHashReporter
)
