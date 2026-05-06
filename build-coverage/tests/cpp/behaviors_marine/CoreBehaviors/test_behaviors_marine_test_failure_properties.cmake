if(DEFINED test_behaviors_marine_test_failure_TESTS)
  set_tests_properties(${test_behaviors_marine_test_failure_TESTS} PROPERTIES LABELS [=[behaviors-marine;BHV_TestFailure;pHelmIvP]=])
endif()
