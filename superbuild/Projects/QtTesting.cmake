option(SUPPRESS_QtTesting_BUILD_OUTPUT "Suppress CGM build output" ON)
mark_as_advanced(SUPPRESS_QtTesting_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_QtTesting_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(QtTesting
  DEPENDS qt
  CMAKE_ARGS
    ${extra_cmake_args}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  ${suppress_build_out}
  )
