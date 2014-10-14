#cgm has to be built in source to work
add_external_project(cgm
  DEPENDS OCE
  BUILD_IN_SOURCE 1
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
