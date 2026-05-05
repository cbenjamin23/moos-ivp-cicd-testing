set(ALOGSCAN_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/app_alogscan)

add_moos_cpp_test(test_alogscan_scan_handler
  SOURCES
    ScanHandlerTest.cpp
    ${ALOGSCAN_SRC}/ScanHandler.cpp
  LIBS
    logutils
    mbutil
  INCLUDES
    ${ALOGSCAN_SRC}
  LABELS
    alogscan
    ScanHandler
)
