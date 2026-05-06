if(DEFINED test_behaviors_marine_timeout_TESTS)
  set_tests_properties(${test_behaviors_marine_timeout_TESTS} PROPERTIES LABELS [=[behaviors-marine;BHV_TimeOut;pHelmIvP]=])
endif()
