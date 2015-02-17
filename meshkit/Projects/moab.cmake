set(extra_deps)
if (BUILD_WITH_CUBIT)
  list(APPEND extra_deps
    cubit)
endif ()

if (BUILD_WITH_MPI)
  list(APPEND extra_deps
    pnetcdf)
endif ()

add_external_project(moab
  DEPENDS hdf5 cgm netcdf zlib ${extra_deps}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>
    -DENABLE_IMESH:BOOL=ON
    -DMOAB_USE_CGM:BOOL=ON
    -DMOAB_USE_NETCDF:BOOL=ON
    -DMOAB_USE_HDF:BOOL=ON
    "-DCUBITROOT:PATH=${CUBITROOT}"
    -DMOAB_USE_MPI:BOOL=${BUILD_WITH_MPI}
  ${suppress_build_out}
)
