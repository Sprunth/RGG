set(extra_deps)
if (BUILD_WITH_CUBIT)
  list(APPEND extra_deps
    cubit)
else ()
  list(APPEND extra_deps
    OCE)
endif ()

add_external_project(cgm
  DEPENDS ${extra_deps}
  CMAKE_ARGS
    ${cgm_cmake_args}
    -DCGM_KCM:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DUSE_MPI:BOOL=${BUILD_WITH_MPI})

add_extra_cmake_args(
  "-DCGM_DIR=<INSTALL_DIR>/lib/cmake/CGM")
