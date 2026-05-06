# Install script for directory: /Users/charlesbenjamin/moos-ivp-cicd-testing/tests/cpp/geometry

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/AngleUtils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/GeomUtils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/ArcUtils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYArc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/CircularUtils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYPrimitiveShapes/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/Dubins/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/TrackProximity/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsPoint/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsPoly/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsSegl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsSeglr/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsCircle/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsVector/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsMarker/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsPulse/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYFormatUtilsWedge/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYViewerShapes/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYConvexGrid/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/XYGridFamily/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/PathUtils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/CPAEngine/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/CPAPlatform/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/FieldArtifacts/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/PolygonGeneration/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/CPAVariants/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/ViewPlugSettings/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/MiscGeometryTools/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/IOAndPopulators/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/ObjectBaseBuild/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/GeoShapesLifecycle/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
