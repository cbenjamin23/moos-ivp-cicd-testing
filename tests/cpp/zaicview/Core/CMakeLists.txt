find_package(OpenGL QUIET)
find_package(FLTK QUIET)

set(ZAIC_APP_SRC ${MOOSIVP_SOURCE_TREE_BASE}/ivp/src)
set(ZAIC_MODEL_SOURCES
  ${ZAIC_APP_SRC}/lib_zaicview/ZAIC_Model.cpp
  ${ZAIC_APP_SRC}/app_zaic_hdg/ZAIC_HDG_Model.cpp
  ${ZAIC_APP_SRC}/app_zaic_spd/ZAIC_SPD_Model.cpp
  ${ZAIC_APP_SRC}/app_zaic_hleq/ZAIC_HLEQ_Model.cpp
  ${ZAIC_APP_SRC}/app_zaic_peak/ZAIC_PEAK_Model.cpp
  ${ZAIC_APP_SRC}/app_zaic_vect/ZAIC_VECT_Model.cpp
  ${ZAIC_APP_SRC}/app_zaic_vect/PopulatorVZAIC.cpp
)
set(ZAIC_INCLUDES
  ${ZAIC_APP_SRC}/lib_zaicview
  ${ZAIC_APP_SRC}/app_zaic_hdg
  ${ZAIC_APP_SRC}/app_zaic_spd
  ${ZAIC_APP_SRC}/app_zaic_hleq
  ${ZAIC_APP_SRC}/app_zaic_peak
  ${ZAIC_APP_SRC}/app_zaic_vect
)

if(APPLE)
  set(ZAIC_PLATFORM_DEFINE OPSYS_IS_OS_X=1)
else()
  set(ZAIC_PLATFORM_DEFINE OPSYS_IS_LINUX=1)
endif()

set(ZAIC_MODEL_SUITE_LABELS
  ZAICModelTest=ZAIC_Model
  ZAICHDGModelTest=ZAIC_Model,ZAIC_HDG_Model,zaic_hdg
  ZAICSPDModelTest=ZAIC_Model,ZAIC_SPD_Model,zaic_spd
  ZAICHLEQModelTest=ZAIC_Model,ZAIC_HLEQ_Model,zaic_hleq
  ZAICPEAKModelTest=ZAIC_Model,ZAIC_PEAK_Model,zaic_peak
  ZAICVectModelTest=ZAIC_Model,ZAIC_VECT_Model,PopulatorVZAIC,zaic_vect
)

add_moos_cpp_test(test_zaicview_models
  SOURCES
    ZAICViewCoreTest.cpp
    ${ZAIC_MODEL_SOURCES}
  LIBS
    ivpbuild
    ivpcore
    ivpsolve
    geometry
    mbutil
  INCLUDES
    ${ZAIC_INCLUDES}
  DEFINES
    ZAICVIEW_ENABLE_MODEL_TESTS
    ${ZAIC_PLATFORM_DEFINE}
  LABELS
    zaicview
  SUITE_LABELS
    ${ZAIC_MODEL_SUITE_LABELS}
)

if(OpenGL_FOUND AND FLTK_FOUND)
  add_moos_cpp_test(test_zaicview_gui
    SOURCES
      ZAICViewCoreTest.cpp
      ${ZAIC_MODEL_SOURCES}
      ${ZAIC_APP_SRC}/lib_zaicview/ZAIC_Viewer.cpp
      ${ZAIC_APP_SRC}/lib_zaicview/ZAIC_GUI.cpp
      ${ZAIC_APP_SRC}/app_zaic_hdg/ZAIC_HDG_GUI.cpp
      ${ZAIC_APP_SRC}/app_zaic_spd/ZAIC_SPD_GUI.cpp
      ${ZAIC_APP_SRC}/app_zaic_hleq/ZAIC_HLEQ_GUI.cpp
      ${ZAIC_APP_SRC}/app_zaic_peak/ZAIC_PEAK_GUI.cpp
      ${ZAIC_APP_SRC}/app_zaic_vect/ZAIC_VECT_GUI.cpp
    LIBS
      ivpbuild
      ivpcore
      ivpsolve
      geometry
      mbutil
      ${OPENGL_LIBRARIES}
      ${FLTK_LIBRARIES}
    INCLUDES
      ${ZAIC_INCLUDES}
      ${FLTK_INCLUDE_DIR}
    DEFINES
      ZAICVIEW_ENABLE_GUI_TESTS
      ${ZAIC_PLATFORM_DEFINE}
    LABELS
      zaicview
      gui
      requires_fltk
      requires_opengl
    SUITE_LABELS
      ZAICViewerTest=ZAIC_Viewer
      ZAICGUITest=ZAIC_GUI
  )
else()
  message(STATUS "Skipping zaicview GUI CTests because OpenGL or FLTK was not found")
endif()
