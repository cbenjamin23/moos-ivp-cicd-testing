if(DEFINED test_dep_behaviors_obstacle_model_TESTS)
  set_tests_properties(${test_dep_behaviors_obstacle_model_TESTS} PROPERTIES LABELS [=[dep_behaviors;ObShipModel;AOF_AvoidObstacle;AOF_AvoidObstacleX;RefineryObAvoid;pHelmIvP]=])
endif()
