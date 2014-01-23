###########################################################################
#
#  Program:   CMBNuclearSuite
#
#  Copyright (c) Kitware Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0.txt
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################

cmake_minimum_required(VERSION 2.8.6)

#-----------------------------------------------------------------------------
project(CMBNuclearSuite)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# See http://cmake.org/cmake/help/cmake-2-8-docs.html#section_Policies for details
#

#Policy CMP0003 deals with how specify link paths, with the policy set to NEW
#we are stating that we shouldn't split link paths into search paths and
#library names, but instead directly link using the full path. You really
#don't want to remove this policy before we set the cmake_minimum_required to
#be 3.X
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

#-----------------------------------------------------------------------------
# Ensure that SUBDIRS is not used in this project:
# (we need in-order processing so the fixup script for the bundle
#  applications works properly...)
#
macro(SUBDIRS)
  message(FATAL_ERROR "error: do not use SUBDIRS in this project. Use ADD_SUBDIRECTORY instead.")
endmacro(SUBDIRS)

set(CMBNuc_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH "${CMBNuc_CMAKE_DIR}" ${CMAKE_MODULE_PATH})

include(cmbNucVersion)

if(NOT APPLE AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  if(NOT "${CMAKE_CXX_FLAGS}" MATCHES "-fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
  if(NOT "${CMAKE_C_FLAGS}" MATCHES "-fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  endif()
endif()

message(STATUS "CMAKE_CXX_COMPILER_ID='${CMAKE_CXX_COMPILER_ID}'")
message(STATUS "CMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'")
message(STATUS "CMAKE_C_FLAGS='${CMAKE_C_FLAGS}'")

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

#-----------------------------------------------------------------------------
# Library mode: SHARED (default) or STATIC
#
set(CMBNuc_LIBRARY_MODE "SHARED")

option(CMBNuc_BUILD_SHARED_LIBS "Build CMBNuclear libraries as shared module." ON)
mark_as_advanced(CMBNuc_BUILD_SHARED_LIBS)
if(NOT CMBNuc_BUILD_SHARED_LIBS)
  set(CMBNuc_LIBRARY_MODE "STATIC")
endif()

#-----------------------------------------------------------------------------
# Set a default build type if none was specified
#
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  mark_as_advanced(CMAKE_BUILD_TYPE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

#-----------------------------------------------------------------------------
# setup output directories and cpack stuff
#

include(CPackCreate)

if(NOT DEFINED LIBRARY_OUTPUT_PATH)
  set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
    "Single output directory for building all libraries.")
else()
  set (LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH} CACHE INTERNAL
    "Single output directory for building all libraries.")
endif()

message ("EXECUTABLE_OUTPUT_PATH=${EXECUTABLE_OUTPUT_PATH}")
if(NOT DEFINED EXECUTABLE_OUTPUT_PATH)
  set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
    "Single output directory for building all executables.")
else()
   set (EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} CACHE INTERNAL
    "Single output directory for building all executables.")
endif()

#-----------------------------------------------------------------------------
# Require VTK and Qt libraries
#

FIND_PACKAGE(VTK)
INCLUDE(${VTK_USE_FILE}) # include UseVTK.cmake
# We need to add the current value of VTK_MODULES_DIR to the module path
# so that when the plugins are built all the modules can be found. Otherwise,
# modules that aren't loaded as direct dependencies of CMBNuclear modules will
# not be found.
list(APPEND CMAKE_MODULE_PATH "${VTK_MODULES_DIR}")

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})  # include UseQt4.cmake

#-----------------------------------------------------------------------------
# Require Remus for exporting
find_package(Remus REQUIRED)
include_directories(${Remus_INCLUDE_DIR})

#-----------------------------------------------------------------------------
# Libs
#
# add_subdirectory(Libs)

#-----------------------------------------------------------------------------
# Application
#
add_subdirectory(Application)
