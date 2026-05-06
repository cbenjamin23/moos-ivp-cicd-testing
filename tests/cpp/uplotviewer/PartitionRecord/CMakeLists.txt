set(UPLOTVIEWER_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uPlotViewer)

add_moos_cpp_test(test_uplotviewer_partition_record
  SOURCES
    PartitionRecordTest.cpp
    ${UPLOTVIEWER_SRC}/PartitionRecord.cpp
  LIBS
    mbutil
  INCLUDES
    ${UPLOTVIEWER_SRC}
  LABELS
    uplotviewer
    PartitionRecord
  SUITE_LABELS
    PartitionRecordTest=PartitionRecord
)
