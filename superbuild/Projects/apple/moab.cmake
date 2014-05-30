

#reset back the cppflag to the pre netcdf values.
#this works since the only project that depends on netcdf is moab
if (build-projects)
  set (cppflags "${pre_netcdf_cpp_flags}")
endif()


#
add_external_project(moab
  DEPENDS hdf5 cgm netcdfcpp
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>
    -DHDF5_DIR:path=<INSTALL_DIR>
    -DCGM_CFG:path=<INSTALL_DIR>/lib/cgm.make
    -DNetCDF_CXX_LIBRARY:path=<INSTALL_DIR>/lib/libnetcdf.dylib
    -DNetCDF_C_LIBRARY:path=<INSTALL_DIR>/lib/libnetcdf.dylib
    -DNetCDF_FORTRAN_LIBRARY:path=<INSTALL_DIR>/lib/libnetcdf.dylib
    -DNetCDF_INCLUDE_DIRECTORIES:path=<INSTALL_DIR>/include
    -DNetCDF_PREFIX:path=<INSTALL_DIR>
)
