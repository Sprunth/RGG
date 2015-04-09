#
add_external_project(moab
  DEPENDS hdf5 netcdf
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=OFF #ON # LGPL  THIS NEEDS TO BE REVERTED, but is required until decl specing is finished
    -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>
    -DENABLE_IMESH:BOOL=ON
    -DHDF5_DIR:path=<INSTALL_DIR>
    -DNetCDF_DIR:path=<INSTALL_DIR>
    -DMOAB_USE_CGM:BOOL=FALSE
    -DMOAB_USE_HDF:BOOL=ON
    -DMOAB_USE_NETCDF:BOOL=ON
)
