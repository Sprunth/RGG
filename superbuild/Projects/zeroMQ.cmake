option(SUPPRESS_ZEROMQ_BUILD_OUTPUT "Suppress ZeroMQ build output" ON)
mark_as_advanced(SUPPRESS_ZEROMQ_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_ZEROMQ_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(zeroMQ
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DZMQ_BUILD_FRAMEWORK:BOOL=OFF
  ${suppress_build_out}
)
