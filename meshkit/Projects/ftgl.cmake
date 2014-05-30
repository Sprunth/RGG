
add_external_project(ftgl
  DEPENDS freetype
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS=ON
  )
