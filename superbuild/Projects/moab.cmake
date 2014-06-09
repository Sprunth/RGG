

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
    -DENABLE_IMESH:BOOL=ON
    -DMOAB_USE_CGM:BOOL=ON
    -DMOAB_USE_NETCDF:BOOL=ON
    -DMOAB_USE_HDF:BOOL=ON
    -DCGM_CFG:path=<INSTALL_DIR>/lib/cgm.make
    -DNetCDF_DIR:path=<INSTALL_DIR>
)
