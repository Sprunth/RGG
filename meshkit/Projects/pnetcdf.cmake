add_external_dummy_project(pnetcdf)

add_extra_cmake_args(
  "-DPNetCDF_DIR=${PNetCDF_DIR}"
  "-DPNetCDF_INCLUDES=${PNetCDF_INCLUDES}"
  "-DPNetCDF_LIBRARIES=${PNetCDF_LIBRARIES}")
