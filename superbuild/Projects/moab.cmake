

#reset back the cppflag to the pre netcdf values.
#this works since the only project that depends on netcdf is moab
if (build-projects)
  set (cppflags "${pre_netcdf_cpp_flags}")
endif()

option(SUPPRESS_MOAB_BUILD_OUTPUT "Suppress MOAB build output" ON)
mark_as_advanced(SUPPRESS_MOAB_BUILD_OUTPUT)

if(SUPPRESS_MOAB_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()


if(ENABLE_meshkit)
  add_external_project(moab
    DEPENDS hdf5 cgm netcdf
    CMAKE_ARGS
      -DBUILD_SHARED_LIBS:BOOL=ON # LGPL
      -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>
      -DHDF5_DIR:path=<INSTALL_DIR>
      -DENABLE_IMESH:BOOL=ON
      -DMOAB_USE_CGM:BOOL=ON
      -DMOAB_USE_NETCDF:BOOL=ON
      -DMOAB_USE_HDF:BOOL=ON
      -DNetCDF_DIR:path=<INSTALL_DIR>
      -DCGM_CFG:path=<INSTALL_DIR>/lib/cgm.make
    ${suppress_build_out}
  )

else()

  add_external_project(moab
    DEPENDS hdf5 netcdfcpp
    CMAKE_ARGS
      -DBUILD_SHARED_LIBS:BOOL=ON # LGPL
      -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>
      -DHDF5_DIR:path=<INSTALL_DIR>
      -DENABLE_IMESH:BOOL=ON
      -DMOAB_USE_CGM:BOOL=OFF
      -DMOAB_USE_NETCDF:BOOL=ON
      -DMOAB_USE_HDF:BOOL=ON
      -DNetCDF_DIR:path=<INSTALL_DIR>
    ${suppress_build_out}
  )

endif()
