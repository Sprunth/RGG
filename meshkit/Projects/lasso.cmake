add_external_project(lasso
  DEPENDS moab cgm zlib
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DLASSO_ENABLE_FORTRAN:BOOL=FALSE
    -DLASSO_ENABLE_MPI:BOOL=${BUILD_WITH_MPI})
