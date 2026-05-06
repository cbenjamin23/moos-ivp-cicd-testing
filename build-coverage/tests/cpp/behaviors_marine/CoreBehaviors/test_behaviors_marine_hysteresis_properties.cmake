if(DEFINED test_behaviors_marine_hysteresis_TESTS)
  set_tests_properties(${test_behaviors_marine_hysteresis_TESTS} PROPERTIES LABELS [=[behaviors-marine;BHV_Hysteresis;pHelmIvP]=])
endif()
