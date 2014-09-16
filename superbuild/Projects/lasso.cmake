add_external_project(lasso
  DEPENDS moab cgm
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --enable-encoding
    --enable-shared
    --with-imesh=<INSTALL_DIR>
    --with-igeom=<INSTALL_DIR>
    --prefix=<INSTALL_DIR>
)

if(ENABLE_meshkit)
  add_external_project_step(lasso-autoconf
    COMMAND ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR>
    DEPENDEES update
    DEPENDERS configure
  )
endif()
