option(SUPPRESS_meshkit_BUILD_OUTPUT "Suppress meshkit build output" ON)
mark_as_advanced(SUPPRESS_meshkit_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_meshkit_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

cmake_dependent_option(BUILD_WITH_CUBIT "Build CGM with CUBIT" OFF
  ENABLE_meshkit OFF)

set(meshkit_args
  BUILD_COMMAND "make -j5 install"
  INSTALL_COMMAND ""
  ${suppress_build_out}
  CMAKE_ARGS
    -DBUILD_MESHKIT_MASTER:BOOL=${BUILD_MESHKIT_MASTER}
    -DBUILD_WITH_CUBIT:BOOL=${BUILD_WITH_CUBIT}
    -DENABLE_ALGS:BOOL=ON
    -DENABLE_SRC:BOOL=ON
    -DENABLE_UTILS:BOOL=ON
    -DENABLE_RGG:BOOL=ON
    -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>/meshkitCubit
    -DCMAKE_PREFIX_PATH:path=<INSTALL_DIR>/meshkitCubit
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCUBITROOT:PATH=${CUBITROOT})
