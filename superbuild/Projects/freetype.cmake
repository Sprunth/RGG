
add_external_project(
  freetype
  DEPENDS zlib
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
                    --prefix=<INSTALL_DIR>
                    --enable-static=no
                     --with-sysroot=<INSTALL_DIR>
) 

add_extra_cmake_args(
  -DFREETYPE_INCLUDE_DIR_freetype2=<INSTALL_DIR>/include/freetype2
  -DFREETYPE_INCLUDE_DIR_ft2build=<INSTALL_DIR>/include
)
