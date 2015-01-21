
add_external_project(ftgl
  DEPENDS freetype
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS=ON
    -DFREETYPE_INCLUDE_DIR_freetype2=<INSTALL_DIR>/include/freetype2
    -DFREETYPE_INCLUDE_DIR_ft2build=<INSTALL_DIR>/include
  )
