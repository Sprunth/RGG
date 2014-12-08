if(BUILD_MESHKIT_MASTER)
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
else()
  add_external_project(meshkit
    DEPENDS moab cgm lasso
    USE_AUTOCONF
    PATCH_COMMAND ${GIT_EXECUTABLE} apply ${CMAKE_SOURCE_DIR}/patches/meshkit.fix_make_watertight.txt
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
endif()
