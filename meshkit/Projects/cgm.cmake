set(cgm_cmake_args)
if (BUILD_WITH_CUBIT)
  list(APPEND cgm_cmake_args
    "-DCUBITROOT:PATH=${CUBITROOT}")
endif ()

add_external_project(cgm
  CMAKE_ARGS
    ${cgm_cmake_args}
    -DCGM_KCM:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DUSE_MPI:BOOL=${BUILD_WITH_MPI})

set_property(GLOBAL APPEND
  PROPERTY
    cgm_CMAKE_ARGS "-DCGM_DIR=<INSTALL_DIR>/lib/cmake/CGM")
