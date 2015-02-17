add_external_dummy_project(netcdf)

add_extra_cmake_args(
  "-DNetCDF_DIR:path=${NetCDF_DIR}")
