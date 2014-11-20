if(BUILD_DOCUMENTATION)
  set(DOC_ARGS -DBUILD_DOCUMENTATION:BOOL=${BUILD_DOCUMENTATION})
endif()

if(BUILD_WITH_CUBIT)
  set(CUBIT_ARGS -DCUBIT_BUILD:BOOL=ON)
endif()

add_external_project_or_just_build_dependencies(nuclearRGG
  DEPENDS remus qt vtk moab QtTesting
  CMAKE_ARGS
    ${extra_cmake_args}
    ${MESH_ARGS}
    ${DOC_ARGS}

    # specify the apple app install prefix. No harm in specifying it for all
    # platforms.
    -DMACOSX_APP_INSTALL_PREFIX:PATH=<INSTALL_DIR>/Applications
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DVTK_MOAB_SUPPORT:BOOL=ON
    ${CUBIT_ARGS}
    -DMOAB_ROOT_DIR:PATH=<INSTALL_DIR>
    -DBUILD_TESTING:BOOL=${BUILD_TESTING}
)
