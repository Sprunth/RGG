add_external_project(meshkit
  DEPENDS moab cgm lasso
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DENABLE_ALGS:BOOL=ON
    -DENABLE_SRC:BOOL=ON
    -DENABLE_UTILS:BOOL=ON
    -DENABLE_RGG:BOOL=ON
)
