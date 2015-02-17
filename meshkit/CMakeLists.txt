cmake_minimum_required(VERSION 2.8.8)

if (APPLE)
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    message(WARNING "Ensure that CMAKE_OSX_SYSROOT, CMAKE_OSX_DEPLOYMENT_TARGET are set correctly")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "By default, build for 64-bit Leopard")
  endif()
endif()


project(MeshKitSuperBuild)
set (SuperBuild_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set (SuperBuild_PROJECTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Projects")
include(CMakeDependentOption)

set (MeshKitSuperBuild_CMAKE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

set (install_location "${CMAKE_CURRENT_BINARY_DIR}/install")
set (download_location "${CMAKE_CURRENT_BINARY_DIR}/downloads")
set (patch_location "${CMAKE_CURRENT_SOURCE_DIR}/patches")
set (prefix_path "${install_location}")

set (platform)
if (APPLE)
  set (platform "apple")
elseif (UNIX)
  set (platform "unix")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
  set (cppflags "${cppflags} -I${install_location}/include")
  set (cflags "${cflags} -fPIC")
  set (cxxflags "${cxxflags} -fPIC")
elseif (WIN32)
  set (platform "win32")
endif()

# Setup CMAKE_MODULE_PATH so that platform specific configurations are processed
# before the generic ones.
set (CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/Projects/${platform}"
  "${CMAKE_CURRENT_SOURCE_DIR}/Projects/"
  "${CMAKE_CURRENT_SOURCE_DIR}/CMake"
  ${CMAKE_MODULE_PATH}
  )

include (ParaViewModules)
include (${CMAKE_CURRENT_SOURCE_DIR}/versions.cmake)

option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )
option ( BUILD_MESHKIT_MASTER "Build Meshkit Master" OFF )
option ( BUILD_WITH_MPI       "Build with MPI support" OFF )

if(BUILD_WITH_CUBIT)
  find_package(CUBIT REQUIRED)
endif()

if (APPLE)
  set (cflags "${cflags} -arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (cppflags "${cppflags} -arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (cxxflags "${cxxflags} -arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  if(BUILD_WITH_CUBIT)
    if(${CMAKE_OSX_ARCHITECTURES} STREQUAL "x86_64")
      message(FATAL_ERROR "For CUBIT Builds CMAKE_OSX_ARCHITECTURES needs to be set to i386")
    endif()

  endif()
endif()

#currently remove mpich2 support to make it easier to build for now
#add_project(mpich2)


include(zlib)
include(szip)
include(hdf5)
include(netcdf)
if (NOT WIN32 AND BUILD_WITH_MPI)
  include(pnetcdf)
endif ()
if(BUILD_WITH_CUBIT)
  include(cubit)
else ()
  include(freetype)
  include(ftgl)
  include(OCE)
endif()
include(cgm)
include(moab)
include(lasso)
include(meshkit)

#call cmb process dependencies to get the ability to do a build
#for developers.
process_dependencies()

INSTALL( DIRECTORY ${CMAKE_BINARY_DIR}/install/bin USE_SOURCE_PERMISSIONS DESTINATION meshkit )
INSTALL( DIRECTORY ${CMAKE_BINARY_DIR}/install/lib USE_SOURCE_PERMISSIONS DESTINATION meshkit )
