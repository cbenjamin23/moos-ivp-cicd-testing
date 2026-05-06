if(DEFINED test_alogtest_log_tester_TESTS)
  set_tests_properties(${test_alogtest_log_tester_TESTS} PROPERTIES LABELS [=[alogtest;LogTest;LogTester]=])
endif()
