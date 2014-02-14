
add_external_project(ftgl
  DEPENDS freetype
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --enable-shared "CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
    --with-ft-prefix=<INSTALL_DIR>
)
