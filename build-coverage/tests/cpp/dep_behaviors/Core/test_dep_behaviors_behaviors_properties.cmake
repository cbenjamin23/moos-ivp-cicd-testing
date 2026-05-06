if(DEFINED test_dep_behaviors_behaviors_TESTS)
  set_tests_properties(${test_dep_behaviors_behaviors_TESTS} PROPERTIES LABELS [=[dep_behaviors;BHV_FixTurn;BHV_RubberBand;BHV_Attractor;BHV_AvoidObstacle;BHV_AvoidObstacleX;BHV_AvoidObstacleV21;pHelmIvP]=])
endif()
