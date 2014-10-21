option(SUPPRESS_LASSO_BUILD_OUTPUT
"Suppress LASSO build output"
ON)
mark_as_advanced(SUPPRESS_LASSO_BUILD_OUTPUT)

if(SUPPRESS_LASSO_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(lasso
  DEPENDS moab cgm
  ${suppress_build_out}
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
