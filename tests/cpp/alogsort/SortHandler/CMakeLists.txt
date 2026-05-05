set(ALOGSORT_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogsort)

add_moos_cpp_test(test_alogsort_sort_handler
  SOURCES
    SortHandlerTest.cpp
    ${ALOGSORT_SRC}/SortHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGSORT_SRC}
  LABELS
    alogsort
    SortHandler
)
