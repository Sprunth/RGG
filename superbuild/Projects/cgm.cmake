
#cgm has to be built in source to work
add_external_project(cgm
  DEPENDS oce
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --with-occ=<INSTALL_DIR>
    --prefix=<INSTALL_DIR>
    --enable-shared
)

add_external_project_step(oce-autoconf
  COMMENT "Running autoreconf for oce"
    COMMAND autoreconf -i <SOURCE_DIR>
    DEPENDEES update
    DEPENDERS configure
  )