if(BUILD_MESHKIT_MASTER)
  add_external_project(lasso
    DEPENDS moab cgm
    PATCH_COMMAND ${GIT_EXECUTABLE} apply ${CMAKE_SOURCE_DIR}/patches/lass.apple.dylib.git_patch.txt
    USE_AUTOCONF
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --enable-encoding
      --enable-shared
      --with-imesh=<INSTALL_DIR>
      --with-igeom=<INSTALL_DIR>
      --prefix=<INSTALL_DIR>
  )
else()
  add_external_project(lasso
    DEPENDS moab cgm
    PATCH_COMMAND ${GIT_EXECUTABLE} apply ${CMAKE_SOURCE_DIR}/patches/lass.apple.dylib.git_patch_release.txt
    USE_AUTOCONF
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --enable-encoding
      --enable-shared
      --with-imesh=<INSTALL_DIR>
      --with-igeom=<INSTALL_DIR>
      --prefix=<INSTALL_DIR>
  )
endif()
