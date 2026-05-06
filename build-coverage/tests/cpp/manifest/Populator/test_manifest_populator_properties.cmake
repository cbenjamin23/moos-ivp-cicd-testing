if(DEFINED test_manifest_populator_TESTS)
  set_tests_properties(${test_manifest_populator_TESTS} PROPERTIES LABELS [=[manifest;Populator_ManifestSet;ManifestFile;LOCFile]=])
endif()
