#this will only modify the cppflags for netcdf as the next project in the tree
#moab will unset the cppflags back to the original values
if (build-projects)
  set (pre_netcdf_cpp_flags ${cppflags})
  set (cppflags "-I${install_location}/include ${cppflags}")
endif()

option(SUPPRESS_NETCDF_BUILD_OUTPUT "Suppress netcdf build output" ON)
mark_as_advanced(SUPPRESS_NETCDF_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_NETCDF_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(netcdf
  DEPENDS hdf5

  PATCH_COMMAND
    # this patch fixes following issues:
    # 1. incorrect configure target
    # 2. properly use the provided szip find package.
    ${CMAKE_COMMAND} -E copy_if_different ${SuperBuild_PROJECTS_DIR}/patches/netcdf.src.CMakeLists.txt
                                          <SOURCE_DIR>/CMakeLists.txt

  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_UTILITIES:BOOL=ON
    -DENABLE_NETCDF:BOOL=ON
    -DUSE_SZIP:BOOL=ON
    -DUSE_HDF5:BOOL=ON
    -DENABLE_NETCDF_4:BOOL=ON
  ${suppress_build_out}
)

add_external_project_step(patch1
  COMMENT   "Fixing missing include files."
  COMMAND   ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/Projects/patches/netcdf.load.c
            <SOURCE_DIR>/ncgen3/load.c
  DEPENDEES update
  DEPENDERS patch)

if( CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "10.6" )
  add_external_project_step(patch2
    COMMENT   "Fixing missing include files."
    COMMAND   ${CMAKE_COMMAND} -E copy_if_different
              ${CMAKE_SOURCE_DIR}/Projects/patches/netcdf.daputil.c
              <SOURCE_DIR>/libdap2/daputil.c
    DEPENDEES update
    DEPENDERS patch1)
endif()
