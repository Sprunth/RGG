option(SUPPRESS_ZEROMQ_BUILD_OUTPUT
       "Suppress ZeroMQ build output"
      ON)
mark_as_advanced(SUPPRESS_ZEROMQ_BUILD_OUTPUT)

set(suppress_build_out)

if(SUPPRESS_ZEROMQ_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(zeroMQ
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
    --enable-shared
    --disable-static
    --prefix=<INSTALL_DIR>
  BUILD_IN_SOURCE 1
  ${suppress_build_out}
)
