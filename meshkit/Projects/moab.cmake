#MOAB supports CMake

get_libraries(szip szip_libraries)
get_libraries(zlib zlib_libraries)
get_libraries(hdf5 hdf5_libraries)
get_libraries(netcdf netcdf_libraries)

#strip the file name and the lib directory for moab
get_filename_component(szip_path ${szip_libraries} PATH)
get_filename_component(zlib_path ${zlib_libraries} PATH)
get_filename_component(hdf5_path ${hdf5_libraries} PATH)
set(netcdf_path ${install_location})

#strip the lib directory
get_filename_component(szip_path "${szip_path}../" PATH)
get_filename_component(zlib_path "${zlib_path}../" PATH)
get_filename_component(hdf5_path "${hdf5_path}../" PATH)

#we are presuming the library for all these projects are
#in a folder called lib, on some systems like
#ubuntu that isn't true, so moab breaks.

#with cgm turned on moab doesn't properly link to cgm, so we need to add a patch step
if(BUILD_MESHKIT_MASTER)
add_external_project(moab
  DEPENDS hdf5 cgm netcdf
  CMAKE_ARGS
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
else(BUILD_MESHKIT_MASTER)
add_external_project(moab
   DEPENDS hdf5 cgm netcdf
  USE_AUTOCONF
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --with-hdf5=${hdf5_path}
    --with-zlib=${zlib_path}
    --with-szip=${szip_path}
    --with-netcdf=${netcdf_path}
    --with-cgm=<INSTALL_DIR>
    --without-damsel
    --without-ccmio
    --enable-dagmc
    --enable-shared
    --enable-dagmc
    "CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
)
endif(BUILD_MESHKIT_MASTER)