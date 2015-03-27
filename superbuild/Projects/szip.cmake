option(SUPPRESS_SZIP_BUILD_OUTPUT "Suppress SZIP build output" ON)
mark_as_advanced(SUPPRESS_SZIP_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_SZIP_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(szip
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
                    --enable-encoding
                    --prefix=<INSTALL_DIR>
  ${suppress_build_out}
)

# any project depending on szip, inherits these cmake variables
add_extra_cmake_args(
    -DSZIP_DIR:PATH=<INSTALL_DIR>
    -DSZIP_LIBRARY:FILEPATH=<INSTALL_DIR>/lib/${CMAKE_SHARED_LIBRARY_PREFIX}sz${CMAKE_SHARED_LIBRARY_SUFFIX}
    -DSZIP_INCLUDE_DIR:FILEPATH=<INSTALL_DIR>/include)
