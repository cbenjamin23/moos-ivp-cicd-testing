set(UHELMSCOPE_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uHelmScope)

add_moos_cpp_test(test_uhelmscope_helm_records
  SOURCES
    BehaviorRecordTest.cpp
    IterBlockHelmTest.cpp
    ${UHELMSCOPE_SRC}/BehaviorRecord.cpp
    ${UHELMSCOPE_SRC}/IterBlockHelm.cpp
  LIBS
    mbutil
  INCLUDES
    ${UHELMSCOPE_SRC}
  LABELS
    uhelmscope
    HelmRecords
  SUITE_LABELS
    BehaviorRecordTest=BehaviorRecord
    IterBlockHelmTest=IterBlockHelm
)
