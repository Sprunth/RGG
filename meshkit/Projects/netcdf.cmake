get_libraries(hdf5 hdf5_libraries)
get_filename_component(hdf5_path ${hdf5_libraries} PATH)
get_filename_component(hdf5_path "${hdf5_path}../" PATH)

if(APPLE)

  add_external_project(netcdf
    DEPENDS hdf5
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/Projects/patches/load.c <SOURCE_DIR>/ncgen3/load.c
    USE_AUTOCONF
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND  <SOURCE_DIR>/configure
    --enable-netcdf4
    --prefix=<INSTALL_DIR>
    "CPPFLAGS=-I${hdf5_path}/include ${cppflags}"
    "LDFLAGS=-L${hdf5_path}/lib -arch ${CMAKE_OSX_ARCHITECTURES}" )

else()

  add_external_project(netcdf
    DEPENDS hdf5
    USE_AUTOCONF
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND CPPFLAGS=-I${hdf5_path}/include LDFLAGS=-L${hdf5_path}/lib <SOURCE_DIR>/configure
    --enable-netcdf4
    --prefix=<INSTALL_DIR> )

endif()


ExternalProject_Get_Property(${name} install_dir)
set_libraries(netcdf ${install_dir}/lib/libnetcdf${CMAKE_SHARED_LIBRARY_SUFFIX})
