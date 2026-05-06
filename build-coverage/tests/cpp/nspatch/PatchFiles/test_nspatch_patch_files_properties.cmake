if(DEFINED test_nspatch_patch_files_TESTS)
  set_tests_properties(${test_nspatch_patch_files_TESTS} PROPERTIES LABELS [=[nspatch;PatchFiles]=])
endif()
