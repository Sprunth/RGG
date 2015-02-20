set(extra_deps)
if (BUILD_WITH_CUBIT)
  list(APPEND extra_deps
    cubit)
  set(use_occ OFF)
else ()
  list(APPEND extra_deps
    OCE)
  set(use_occ ON)
endif ()

add_external_project(cgm
  DEPENDS ${extra_deps}
  CMAKE_ARGS
    -DCGM_OCC:BOOL=${use_occ}
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DUSE_MPI:BOOL=${BUILD_WITH_MPI})

add_extra_cmake_args(
  "-DCGM_DIR=<INSTALL_DIR>/lib/cmake/CGM")
