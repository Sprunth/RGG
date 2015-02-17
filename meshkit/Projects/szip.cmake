add_external_dummy_project(szip)

add_extra_cmake_args(
  "-DSZIP_LIBRARIES:FILEPATH=${SZIP_LIBRARIES}")
