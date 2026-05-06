# Install script for directory: /Users/charlesbenjamin/moos-ivp-cicd-testing/tests/cpp

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
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/_deps/googletest-build/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geometry/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ivpbuild/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ivpcore/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ivpsolve/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/bhvutil/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/behaviors/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/behaviors_marine/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/behaviors_colregs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/dep_behaviors/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/mbutil/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogcheck/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogavg/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogbin/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogcat/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogclip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/aloghelm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/aloggrep/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogeplot/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogeval/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogcd/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogiter/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogload/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogmhash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogpare/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogpick/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogrm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogscan/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogsplit/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogsort/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogtest/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/alogtm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/gen_moos_app/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/nspatch/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/nsplug/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/pechovar/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/pickpos/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/pluck/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/projfield/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/prealm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/tagrep/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/logutils/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/apputil/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/manifest/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/genutil/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ufield/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/turngeo/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/polar/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/geodaid/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/survey/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/realm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/encounters/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/marine_pid/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/helmivp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/isay/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/logic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/uhelmscope/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/contacts/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/evalutil/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/obstacles/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ucommand/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ufldbeaconrangesensor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ufldcollisiondetect/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/uflddelve/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/uplotviewer/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/utimerscript/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/ipfview/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/marineview/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/zaicview/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/charlesbenjamin/moos-ivp-cicd-testing/build-coverage/tests/cpp/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
