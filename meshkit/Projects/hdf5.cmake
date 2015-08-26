add_external_dummy_project(hdf5)

add_extra_cmake_args(
  -DHDF5_DIR:PATH=${HDF5_DIR}
  -DHDF5_LIBRARIES:FILEPATH=${HDF5_LIBRARIES}
  -DHDF5_INCLUDE_DIR:PATH=${HDF5_INCLUDE_DIR})
