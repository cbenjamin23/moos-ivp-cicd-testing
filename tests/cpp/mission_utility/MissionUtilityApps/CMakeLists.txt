set(PMISSIONHASH_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/pMissionHash)
set(UMAYFINISH_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uMayFinish)

add_moos_cpp_test(test_mission_utility_apps
  SOURCES
    MissionUtilityAppsTest.cpp
    ${PMISSIONHASH_SRC}/MissionHash_MOOSApp.cpp
    ${UMAYFINISH_SRC}/MayFinish.cpp
  LIBS
    apputil
    mbutil
    ${MOOS_LIBRARIES}
  INCLUDES
    ${PMISSIONHASH_SRC}
    ${UMAYFINISH_SRC}
  LABELS
    mission_utility
  SUITE_LABELS
    MissionHashAppTest=pMissionHash
    MayFinishConfigTest=uMayFinish
)
