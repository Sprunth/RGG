option(SUPPRESS_LASSO_BUILD_OUTPUT
"Suppress LASSO build output"
ON)
mark_as_advanced(SUPPRESS_LASSO_BUILD_OUTPUT)

if(SUPPRESS_LASSO_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

if(BUILD_MESHKIT_MASTER)
  add_external_project(lasso
    DEPENDS moab cgm
    BUILD_IN_SOURCE 1
    ${suppress_build_out}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --enable-shared
      --with-imesh=<INSTALL_DIR>
      --with-igeom=<INSTALL_DIR>
      --prefix=<INSTALL_DIR>
      --disable-fortran
  )
else()
  add_external_project(lasso
    DEPENDS moab cgm
    BUILD_IN_SOURCE 1
    ${suppress_build_out}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --enable-encoding
      --enable-shared
      --with-imesh=<INSTALL_DIR>
      --with-igeom=<INSTALL_DIR>
      --prefix=<INSTALL_DIR>
      --disable-fortran
  )
endif()

if(ENABLE_meshkit)
  add_external_project_step(lasso-autoconf
    COMMAND ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR> &> <SOURCE_DIR>/AUTOTOOLS_MESSAGES.txt
    DEPENDEES update
    DEPENDERS configure
  )
endif()
