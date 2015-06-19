# Consolidates platform independent stub for rgg.bundle.cmake files.

# We hardcode the version numbers since we cannot determine versions during
# configure stage.
include (${CMAKE_SOURCE_DIR}/../CMake/cmbNucVersion.cmake)

include (paraview_version)

# Enable CPack packaging.
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "rgg Suite of Tools.")
set(CPACK_PACKAGE_NAME "rgg")
set(CPACK_PACKAGE_VENDOR "Kitware, Inc.")
set(CPACK_RESOURCE_FILE_LICENSE
    "${CMBSuperBuild_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${rgg_version_major})
set(CPACK_PACKAGE_VERSION_MINOR ${rgg_version_minor})
if (rgg_version_suffix)
  set(CPACK_PACKAGE_VERSION_PATCH ${rgg_version_suffix})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY
      "NuclearRGG ${CPACK_PACKAGE_VERSION_PATCH}")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME
      "NuclearRGG-${CPACK_PACKAGE_VERSION_PATCH}")
else()
  set(CPACK_PACKAGE_VERSION_PATCH ${rgg_version_patch})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY
      "NuclearRGG ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME
      "NuclearRGG-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
endif()

if(APPLE)
  set(CPACK_SYSTEM_NAME OSX-${CMAKE_OSX_DEPLOYMENT_TARGET}-${CMAKE_OSX_ARCHITECTURES})
elseif(NOT DEFINED CPACK_SYSTEM_NAME)
  set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
endif()

if(${CPACK_SYSTEM_NAME} MATCHES Windows)
  if(CMAKE_CL_64)
    set(CPACK_SYSTEM_NAME win64)
  else(CMAKE_CL_64)
    set(CPACK_SYSTEM_NAME win32)
  endif()
endif()
if(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
  set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
endif()

#set the programs we want to install
set(rgg_programs_to_install
     RGGNuclear
    )


#sort the list so the package order is reasonable and deducible to the user
list(SORT rgg_programs_to_install)

# Don't import CPack yet, let the platform specific code get another chance at
# changing the variables.
