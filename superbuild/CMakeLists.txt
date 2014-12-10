cmake_minimum_required(VERSION 2.8.9)

if (APPLE)
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    message(WARNING "Ensure that CMAKE_OSX_SYSROOT, CMAKE_OSX_DEPLOYMENT_TARGET are set correctly")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "By default, build for 64-bit Leopard")
  endif()
endif()

project(CMBSuperBuild)
set (SuperBuild_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set (SuperBuild_PROJECTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Projects")
set (SuperBuild_BINARY_DIR ${CMBSuperBuild_BINARY_DIR})

set (platform)
if (APPLE)
  set (platform "apple")
elseif (UNIX)
  set (platform "unix")
elseif (WIN32)
  set (platform "win32")
endif()

#-----------------------------------------------------------------------------
# Cross compilation
set (CROSS_BUILD_STAGE "HOST" CACHE STRING
  "Cross compilation stage: either HOST(ie no crosscompile)|TOOLS|CROSS")
set_property(CACHE CROSS_BUILD_STAGE PROPERTY STRINGS
   "HOST" "TOOLS" "CROSS")

option(BUILD_SHARED_LIBS "Whether to build shared libs by default" OFF)

#-----------------------------------------------------------------------------
# Setup CMAKE_MODULE_PATH so that platform specific configurations are processed
# before the generic ones.
string(TOLOWER "${CROSS_BUILD_STAGE}" lstage)
set (CMAKE_MODULE_PATH
  "${SuperBuild_PROJECTS_DIR}/${platform}/${lstage}"
  "${SuperBuild_PROJECTS_DIR}/${platform}"
  "${SuperBuild_PROJECTS_DIR}/common"
  "${SuperBuild_PROJECTS_DIR}"
  "${SuperBuild_CMAKE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  ${CMAKE_MODULE_PATH})

#-----------------------------------------------------------------------------
if (NOT CROSS_BUILD_STAGE STREQUAL "HOST")
  include(CrossCompilationMacros)

  #ask user to say what machine they are compiling onto
  #so we can get the right environment settings
  query_target_machine()

  if (CROSS_BUILD_STAGE STREQUAL "CROSS")
    #configure the platform dependent settings
    #64bit_build, static_only, mpi search path
    do_cross_platform_settings()

    #find location of hosttools or host build to get compilation tools from
    find_hosttools()
  endif()
endif()

include(CMakeDependentOption)

#-----------------------------------------------------------------------------
# Collect information about the build platform.
include(CheckTypeSize)

if (NOT CROSS_BUILD_STAGE STREQUAL "CROSS")
  check_type_size(void* VOID_PTR_SIZE BUILTIN_TYPES_ONLY)
  if (VOID_PTR_SIZE EQUAL 8)
    set (64bit_build TRUE)
  else ()
    set (64bit_build FALSE)
  endif()
endif()

# Set suffix to be used for generating archives. This ensures that the package
# files have decent names that we can directly upload to the website.
set (package_suffix "${CMAKE_SYSTEM_NAME}")
if (64bit_build)
  set (package_suffix "${package_suffix}-64bit")
else ()
  set (package_suffix "${package_suffix}-32bit")
endif()

include(paraview_version)

option(TRUST_SVN_CERTIFICATES_AUTOMATICALLY
  "Trust SVN certificates automatically" ON)
mark_as_advanced(TRUST_SVN_CERTIFICATES_AUTOMATICALLY)

#-----------------------------------------------------------------------------
# Setup some standard variables that control various locations and flags.
set (install_location "${CMAKE_CURRENT_BINARY_DIR}/install")
set (download_location "${CMAKE_CURRENT_BINARY_DIR}/downloads"
     CACHE PATH "Location where source tar-balls are (to be) downloaded.")
mark_as_advanced(download_location)

if (WIN32)
  set (ldflags)
  set (cppflags)
  set (cxxflags)
  set (ld_library_path)
else ()
  # FIXME: currently, if any of these are empty, we have build issues on Unix.
  set (ldflags "$ENV{LDFLAGS} -L${install_location}/lib")
  if (NOT CROSS_BUILD_STAGE STREQUAL "CROSS")
    set (cppflags "$ENV{CPPFLAGS} -fPIC")
    set (cxxflags "$ENV{CXXFLAGS} -fPIC")
    set (cflags "$ENV{CFLAGS} -fPIC")
  endif()
  set (ld_library_path "${install_location}/lib:$ENV{LD_LIBRARY_PATH}")
endif()

if (UNIX AND NOT APPLE)
  # I am not exactly sure about the cause of these issues and how to cleanly
  # overcome them, however, on some linuxes when Cmake is built shared, we end
  # if we set LD_LIBRARY_PATH to the install/lib dir, we end up with errors
  # due to conflicts with system libz. So for now, I am making this an option.
  # By default we will let the superbuild scripts set LD_LIBRARY_PATH. However
  # if users get errors like: libz.so.1: no version information available, then
  # users should turn this flag off.
  option(PASS_LD_LIBRARY_PATH_FOR_BUILDS
         "Pass LD_LIBRARY_PATH to build scripts."
         ON)
  mark_as_advanced(PASS_LD_LIBRARY_PATH_FOR_BUILDS)
endif()

if (APPLE)
  set (cflags "${cflags} -arch x86_64 -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (cppflags "${cppflags} -arch x86_64 -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (cxxflags "${cxxflags} -arch x86_64 -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} --sysroot=${CMAKE_OSX_SYSROOT} ")
  set (ldflags "${ldflags} -Wl,-headerpad,4000")
endif()
set (prefix_path "${install_location}")

# merge in default/user-specified CMake flags.
set (ldflags "${CMAKE_SHARED_LINKER_FLAGS} ${ldflags}")
set (cppflags "${CMAKE_CXX_FLAGS} ${cppflags}")
set (cxxflags "${CMAKE_CXX_FLAGS} ${cxxflags}")
set (cflags "${CMAKE_C_FLAGS} ${cflags}")

string (STRIP "${ldflags}" ldflags)
string (STRIP "${cppflags}" cppflags)
string (STRIP "${cxxflags}" cxxflags)
string (STRIP "${cflags}"  cflags)

file(WRITE "${CMAKE_BINARY_DIR}/env.cmake" "#AUTOMATICALLY GENERATED FILE. This is used the set environment variables at configure time for cmake projects\n")
if(UNIX AND NOT APPLE AND PASS_LD_LIBRARY_PATH_FOR_BUILDS)
  file(APPEND "${CMAKE_BINARY_DIR}/env.cmake" "set( ENV{LD_LIBRARY_PATH} \"${ld_library_path}\")\n")
endif()

#-----------------------------------------------------------------------------
## Top-level options. These must be limited to a commonly used subset with
## default values being used for building ParaView binaries.
#option(PV_COMMAND_LINE_TOOLS_ONLY
#  "Turn this ON to build command lines tools alone." OFF)

if (WIN32)
  if ( (NOT "${CMAKE_GENERATOR}" MATCHES "^NMake.*$") OR
       (NOT "${CMAKE_GENERATOR}" MATCHES "^Visual Studio.*$"))
  # not use VS environment. We need to be pointed to nmake and devenv paths
  # since they are needed when building tools (qt, python, etc.)
    find_program(DEVENV_PATH devenv)
    find_program(NMAKE_PATH nmake)
    mark_as_advanced(DEVENV_PATH NMAKE_PATH)
  endif()
  # Freetype uses its own build system on win32...
  find_program(FTJAM_EXECUTABLE jam)
  mark_as_advanced(FTJAM_EXECUTABLE)
  if (freetype_ENABLED)
    if (NOT FTJAM_EXECUTABLE)
      message(FATAL_ERROR "FreeType's 'jam' build tool not found! Download it from http://sourceforge.net/projects/freetype/files/ftjam/2.5.2/ and set FTJAM_EXECUTABLE appropriately.")
    endif()
  endif()
endif()
#-----------------------------------------------------------------------------
include (CMBModules)

include (versions)

#------------------------------------------------------------------------------
# enable testing support.
configure_file(
  "${SuperBuild_CMAKE_DIR}/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake" @ONLY)
include(CTest)
#------------------------------------------------------------------------------

#-----------------------------------------------------------------------------
set (CM_PROJECTS_ALL)

option(BUILD_DOCUMENTATION "Build the RGG Documentation" OFF)
option(BUILD_MESHKIT_MASTER "Build Meshkit Master" OFF)

#must before any including of projects
SetupCMBSuperBuild()

# Include all optional projects. These are the ones user can pick and choose.
# Some of these allow using system libraries too.
if (CROSS_BUILD_STAGE STREQUAL "CROSS")
  include(boost)
elseif(CROSS_BUILD_STAGE STREQUAL "TOOLS")
  include(boost)
else()
  include(boost)
  include(hdf5)
  include(freetype)
  include(libxml2)
  include(png)
  include(qt)
endif()

#RGG dependencies

include(zlib)
include(zeroMQ)
include(remus)
include(vtk)

if(ENABLE_meshkit)
  include(OCE)
  include(cgm)
  include(lasso)
endif()

include(meshkit)

if(APPLE AND BUILD_WITH_CUBIT)
  include(meshkit32bit)
  set(ENABLE_meshkit32bit ON)
  mark_as_advanced(ENABLE_meshkit32bit)
  #mark_as_advanced(ENABLE_meshkit32bit ON)
elseif(UNIX AND BUILD_WITH_CUBIT)
  set (ld_library_path "${install_location}/lib:${CUBIT_PATH}/bin:$ENV{LD_LIBRARY_PATH}")
endif()


include(netcdf)
include(ftgl)
include(freetype)
include(netcdfcpp)
include(szip)
include(moab)

if (UNIX AND NOT APPLE)
    include(fontconfig)
endif()

include(QtTesting)
include(nuclearRGG)

include(nuclearRGG.bundle OPTIONAL)

# paraview.sdk is a module that is used install the ParaView SDK.
# Simply enabling this module will result in the ParaView SDK to be
# installed by "make install". Note that this is not always supported.
include(paraviewsdk OPTIONAL)

#call cmb process dependencies to get the ability to do a build
#for developers.
cmb_process_dependencies()

SET (CMAKE_INSTALL_COMPONENT runtime)

#-------------------------------------------------------------------------
#-------------------------------------------------------------------------

#INSTALL( DIRECTORY ${CMAKE_BINARY_DIR}/install/bin DESTINATION rgg_nuclear )

option(NIGHTLY_BINARY_TESTING "Run Nightly-Binary Nuclear RGG Testing" OFF)
if (NIGHTLY_BINARY_TESTING)
  add_subdirectory(tests)
endif()
