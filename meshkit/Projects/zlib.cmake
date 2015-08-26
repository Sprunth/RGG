add_external_dummy_project(zlib)

add_extra_cmake_args(
  "-DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY}")
