set(UFLDDELVE_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uFldDelve)

add_moos_cpp_test(test_uflddelve_trans_record
  SOURCES
    TransRecordTest.cpp
    ${UFLDDELVE_SRC}/TransRecord.cpp
  INCLUDES
    ${UFLDDELVE_SRC}
  LABELS
    uflddelve
    TransRecord
  SUITE_LABELS
    TransRecordTest=TransRecord
)
