include(FetchContent)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

function(add_moos_cpp_test target)
  set(options)
  set(one_value_args)
  set(multi_value_args SOURCES LIBS LABELS)
  cmake_parse_arguments(MOOS_TEST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  add_executable(${target} ${MOOS_TEST_SOURCES})
  target_link_libraries(${target}
    PRIVATE
      GTest::gtest_main
      ${MOOS_TEST_LIBS}
  )
  target_include_directories(${target}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/tests/cpp/support
  )

  gtest_add_tests(TARGET ${target}
    TEST_PREFIX "${target}."
    TEST_LIST ${target}_TESTS
  )
  set_tests_properties(${${target}_TESTS}
    PROPERTIES LABELS "${MOOS_TEST_LABELS}"
  )
endfunction()

add_subdirectory(geometry/AngleUtils)
add_subdirectory(geometry/GeomUtils)
add_subdirectory(geometry/ArcUtils)
add_subdirectory(geometry/XYArc)
add_subdirectory(geometry/CircularUtils)
add_subdirectory(geometry/XYPrimitiveShapes)
add_subdirectory(geometry/Dubins)
add_subdirectory(geometry/TrackProximity)
add_subdirectory(geometry/XYFormatUtilsPoint)
add_subdirectory(geometry/XYFormatUtilsPoly)
add_subdirectory(geometry/XYFormatUtilsSegl)
add_subdirectory(geometry/XYFormatUtilsSeglr)
add_subdirectory(geometry/XYFormatUtilsCircle)
add_subdirectory(geometry/XYFormatUtilsVector)
add_subdirectory(geometry/XYFormatUtilsMarker)
add_subdirectory(geometry/XYFormatUtilsPulse)
add_subdirectory(geometry/XYFormatUtilsWedge)
add_subdirectory(geometry/XYViewerShapes)
add_subdirectory(geometry/XYConvexGrid)
add_subdirectory(geometry/XYGridFamily)
add_subdirectory(geometry/PathUtils)
add_subdirectory(geometry/CPAEngine)
add_subdirectory(geometry/CPAPlatform)
add_subdirectory(geometry/FieldArtifacts)
add_subdirectory(geometry/PolygonGeneration)
add_subdirectory(geometry/CPAVariants)
add_subdirectory(geometry/ViewPlugSettings)
add_subdirectory(geometry/MiscGeometryTools)
add_subdirectory(geometry/IOAndPopulators)
add_subdirectory(geometry/ObjectBaseBuild)
add_subdirectory(geometry/GeoShapesLifecycle)

add_subdirectory(ivpbuild/AOF)
add_subdirectory(ivpbuild/ZAIC)
add_subdirectory(ivpbuild/PDMapAndEncoding)

add_subdirectory(bhvutil)
add_subdirectory(mbutil)
add_subdirectory(logic)
add_subdirectory(contacts)
add_subdirectory(evalutil)
add_subdirectory(obstacles)
add_subdirectory(ucommand)
