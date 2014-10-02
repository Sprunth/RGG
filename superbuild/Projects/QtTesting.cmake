add_external_project(QtTesting
  DEPENDS qt
  CMAKE_ARGS
    ${extra_cmake_args}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  )
