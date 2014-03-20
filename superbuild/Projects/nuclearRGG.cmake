option( BUILD_WITH_MOAB "Build the MOAB support" OFF )

if(BUILD_WITH_MOAB)
add_external_project_or_just_build_dependencies(nuclearRGG
  DEPENDS remus qt vtk moab
  CMAKE_ARGS
    ${extra_cmake_args}
    
    # specify the apple app install prefix. No harm in specifying it for all
    # platforms.
    -DMACOSX_APP_INSTALL_PREFIX:PATH=<INSTALL_DIR>/Applications
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DVTK_DIR:PATH=${CMAKE_BINARY_DIR}/vtk/src/vtk-build
    -DVTK_MOAB_SUPPORT:BOOL=ON
    -DMOAB_ROOT_DIR:PATH=<INSTALL_DIR>
)

else()
add_external_project_or_just_build_dependencies(nuclearRGG
  DEPENDS remus qt vtk
  CMAKE_ARGS
    ${extra_cmake_args}
    
    # specify the apple app install prefix. No harm in specifying it for all
    # platforms.
    -DMACOSX_APP_INSTALL_PREFIX:PATH=<INSTALL_DIR>/Applications
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DVTK_DIR:PATH=${CMAKE_BINARY_DIR}/vtk/src/vtk-build
)
endif()
