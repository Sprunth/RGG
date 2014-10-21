#cgm has to be built in source to work

option(SUPPRESS_CGM_BUILD_OUTPUT
"Suppress CGM build output"
ON)
mark_as_advanced(SUPPRESS_CGM_BUILD_OUTPUT)

if(SUPPRESS_CGM_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(cgm
  DEPENDS OCE
  BUILD_IN_SOURCE 1
  ${suppress_build_out}
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --with-occ=${OCE_DIR}
    --prefix=<INSTALL_DIR>
    --enable-shared
)

if(ENABLE_meshkit)
  add_external_project_step(oce-autoconf
    COMMENT "Running autoreconf for oce"
      COMMAND ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR>
      DEPENDEES update
      DEPENDERS configure
  )
endif()
