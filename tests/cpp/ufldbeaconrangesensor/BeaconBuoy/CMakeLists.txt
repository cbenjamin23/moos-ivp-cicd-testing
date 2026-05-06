set(UFLDBEACONRANGESENSOR_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src/uFldBeaconRangeSensor)

add_moos_cpp_test(test_ufldbeaconrangesensor_beacon_buoy
  SOURCES
    BeaconBuoyTest.cpp
    ${UFLDBEACONRANGESENSOR_SRC}/BeaconBuoy.cpp
  LIBS
    mbutil
  INCLUDES
    ${UFLDBEACONRANGESENSOR_SRC}
  LABELS
    ufldbeaconrangesensor
    BeaconBuoy
  SUITE_LABELS
    BeaconBuoyTest=BeaconBuoy
)
