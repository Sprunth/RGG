option(SUPPRESS_NETCDF_BUILD_OUTPUT "Suppress netcdf build output" ON)
mark_as_advanced(SUPPRESS_NETCDF_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_NETCDF_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

set(cmake_args)
if (BUILD_MESHKIT_WITH_MPI)
  list(APPEND cmake_args
    -DENABLE_PARALLEL:BOOL=ON)
endif ()

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
    -DENABLE_DAP:BOOL=OFF
    -DENABLE_TESTS:BOOL=OFF
    ${cmake_args}
  ${suppress_build_out}
)

add_extra_cmake_args(
  "-DNetCDF_DIR:path=<INSTALL_DIR>")
