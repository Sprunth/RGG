#MOAB supports CMake

get_libraries(szip szip_libraries)
get_libraries(zlib zlib_libraries)
get_libraries(hdf5 hdf5_libraries)
get_libraries(netcdf netcdf_libraries)

#strip the file name and the lib directory for moab
get_filename_component(szip_path ${szip_libraries} PATH)
get_filename_component(zlib_path ${zlib_libraries} PATH)
get_filename_component(hdf5_path ${hdf5_libraries} PATH)
get_filename_component(netcdf_path ${netcdf_libraries} PATH)

#strip the lib directory
get_filename_component(szip_path "${szip_path}../" PATH)
get_filename_component(zlib_path "${zlib_path}../" PATH)
get_filename_component(hdf5_path "${hdf5_path}../" PATH)
get_filename_component(netcdf_path "${netcdf_path}../" PATH)

#we are presuming the library for all these projects are
#in a folder called lib, on some systems like
#ubuntu that isn't true, so moab breaks.

#with cgm turned on moab doesn't properly link to cgm, so we need to add a patch step
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
    --enable-shared
    --enable-dagmc
    "CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
)
