
#find hdf5
find_package(HDF5 REQUIRED)

# add dummy target so that we can attach properties and dependencies work properly
add_dummy_external_project(hdf5)

#explicitly setup the include dir and libraries since add_external_project
#isn't called
set_include_dir(hdf5 ${HDF5_INCLUDE_DIRS})
set_libraries(hdf5 ${HDF5_LIBRARIES})