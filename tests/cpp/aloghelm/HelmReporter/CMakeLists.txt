set(ALOGHELM_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_aloghelm)

add_moos_cpp_test(test_aloghelm_helm_reporter
  SOURCES
    HelmReporterTest.cpp
    ${ALOGHELM_SRC}/HelmReporter.cpp
  LIBS
    behaviors
    helmivp
    ivpbuild
    ivpcore
    logutils
    mbutil
  INCLUDES
    ${ALOGHELM_SRC}
  LABELS
    aloghelm
    HelmReporter
)
