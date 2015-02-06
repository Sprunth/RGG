add_external_project(meshkit
    DEPENDS moab cgm lasso
    USE_AUTOCONF
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
      --with-itaps=<INSTALL_DIR>
      --enable-algs
      --enable-optimize
      --disable-src
      --enable-utils
      --enable-rgg
      --enable-shared
      "CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
  )
