option(DEP_BEHAVIORS_SOURCE_COVERAGE
  "Compile lib_dep_behaviors sources into the dep_behaviors tests with coverage flags"
  OFF)

set(DEP_BEHAVIORS_SOURCES)
set(DEP_BEHAVIORS_LIBRARY dep_behaviors)

if(DEP_BEHAVIORS_SOURCE_COVERAGE)
  set(DEP_BEHAVIORS_LIBRARY)
  set(DEP_BEHAVIORS_SOURCE_DIR
    "${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/lib_dep_behaviors")
  set(DEP_BEHAVIORS_SOURCES
    "${DEP_BEHAVIORS_SOURCE_DIR}/BHV_FixTurn.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/BHV_RubberBand.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/BHV_Attractor.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/BHV_AvoidObstacle.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/BHV_AvoidObstacleX.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/BHV_AvoidObstacleV21.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/ObShipModel.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/AOF_AvoidObstacle.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/AOF_AvoidObstacleX.cpp"
    "${DEP_BEHAVIORS_SOURCE_DIR}/RefineryObAvoid.cpp")
endif()

set(DEP_BEHAVIORS_COMMON_LIBS
  ${DEP_BEHAVIORS_LIBRARY}
  behaviors
  helmivp
  bhvutil
  logic
  ivpbuild
  ivpcore
  contacts
  geometry
  mbutil
)

add_moos_cpp_test(test_dep_behaviors_behaviors
  SOURCES
    DepBehaviorConfigTest.cpp
    ${DEP_BEHAVIORS_SOURCES}
  LIBS
    ${DEP_BEHAVIORS_COMMON_LIBS}
  LABELS
    dep_behaviors
    BHV_FixTurn
    BHV_RubberBand
    BHV_Attractor
    BHV_AvoidObstacle
    BHV_AvoidObstacleX
    BHV_AvoidObstacleV21
    pHelmIvP
)

add_moos_cpp_test(test_dep_behaviors_obstacle_model
  SOURCES
    ObstacleModelAndAOFTest.cpp
    ${DEP_BEHAVIORS_SOURCES}
  LIBS
    ${DEP_BEHAVIORS_COMMON_LIBS}
  LABELS
    dep_behaviors
    ObShipModel
    AOF_AvoidObstacle
    AOF_AvoidObstacleX
    RefineryObAvoid
    pHelmIvP
)

if(DEP_BEHAVIORS_SOURCE_COVERAGE)
  foreach(target
      test_dep_behaviors_behaviors
      test_dep_behaviors_obstacle_model)
    target_compile_options(${target} PRIVATE --coverage -O0 -g)
    target_link_options(${target} PRIVATE --coverage)
  endforeach()
endif()
