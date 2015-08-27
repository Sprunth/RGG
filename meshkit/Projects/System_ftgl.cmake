
#find ftgl
find_package(FTGL REQUIRED)

# add dummy target so that we can attach properties and dependencies work properly
add_dummy_external_project(ftgl)

add_extra_cmake_args(
  -DFTGL_INCLUDE_DIR:PATH=${FTGL_INCLUDE_DIRS})
