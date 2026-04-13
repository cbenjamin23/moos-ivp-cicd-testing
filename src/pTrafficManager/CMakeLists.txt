#--------------------------------------------------------
# The CMakeLists.txt for:                  pTrafficManager
#--------------------------------------------------------

SET(SRC
  TrafficManager.cpp
  TrafficManager_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pTrafficManager ${SRC})

TARGET_LINK_LIBRARIES(pTrafficManager
  ${MOOS_LIBRARIES}
  apputil
  mbutil
  geometry
  m
  pthread)
