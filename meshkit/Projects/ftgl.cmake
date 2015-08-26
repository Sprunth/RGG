add_external_project(ftgl
  DEPENDS freetype
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS=OFF
  )

add_extra_cmake_args(
  -DFTGL_INCLUDE_DIR:PATH=<INSTALL_DIR>/include)
