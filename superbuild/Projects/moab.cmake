option(SUPPRESS_MOAB_BUILD_OUTPUT "Suppress MOAB build output" ON)
mark_as_advanced(SUPPRESS_MOAB_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_MOAB_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(moab
  DEPENDS hdf5 netcdfcpp
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON # LGPL
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DHDF5_DIR:PATH=<INSTALL_DIR>
    -DENABLE_IMESH:BOOL=ON
    -DMOAB_USE_CGM:BOOL=OFF
    -DMOAB_USE_NETCDF:BOOL=ON
    -DMOAB_USE_HDF:BOOL=ON
    -DNetCDF_DIR:PATH=<INSTALL_DIR>
  ${suppress_build_out}
)
