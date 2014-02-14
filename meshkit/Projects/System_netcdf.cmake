if(NOT USE_SYSTEM_HDF5)
  message(FATAL_ERROR "Must use system HDF5 with system NetCDF")
endif()

#find NetCDF
find_package(NetCDF REQUIRED)

# add dummy target so that we can attach properties and dependencies work properly
add_dummy_external_project(netcdf)

#explicitly setup the include dir since add_external_project
#isn't called
set_include_dir(netcdf ${NETCDF_INCLUDE_DIRS})

set_libraries(netcdf ${NETCDF_LIBRARIES})