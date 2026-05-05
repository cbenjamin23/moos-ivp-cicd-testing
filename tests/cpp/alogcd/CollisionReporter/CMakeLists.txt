set(ALOGCD_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogcd)

add_moos_cpp_test(test_alogcd_collision_reporter
  SOURCES
    CollisionReporterTest.cpp
    ${ALOGCD_SRC}/CollisionReporter.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGCD_SRC}
  LABELS
    alogcd
    CollisionReporter
)
