

add_external_project(netcdfcpp
  DEPENDS netcdf
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --prefix=<INSTALL_DIR>
    --enable-shared
    --disable-static
)
