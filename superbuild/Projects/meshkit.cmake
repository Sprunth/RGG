option(SUPPRESS_MESHKIT_BUILD_OUTPUT "Suppress MESHKIT build output" ON)
mark_as_advanced(SUPPRESS_MESHKIT_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_MESHKIT_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(meshkit
  ${suppress_build_out}
  DEPENDS moab cgm lasso
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DENABLE_ALGS:BOOL=ON
    -DENABLE_SRC:BOOL=ON
    -DENABLE_UTILS:BOOL=ON
    -DENABLE_RGG:BOOL=ON
)
