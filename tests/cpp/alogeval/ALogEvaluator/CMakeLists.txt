set(ALOGEVAL_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogeval)

add_moos_cpp_test(test_alogeval_alog_evaluator
  SOURCES
    ALogEvaluatorTest.cpp
    ${ALOGEVAL_SRC}/ALogEvaluator.cpp
  LIBS
    evalutil
    apputil
    logic
    logutils
    mbutil
  INCLUDES
    ${ALOGEVAL_SRC}
  LABELS
    alogeval
    ALogEvaluator
)
