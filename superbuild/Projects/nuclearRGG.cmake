option( BUILD_WITH_MOAB "Build the MOAB support" OFF )

if(BUILD_MESHKIT)
  option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )

  if(BUILD_WITH_CUBIT)
      set(CUBIT_PATH CACHE PATH "Location of the CUBIT Libraries")
      if(NOT IS_DIRECTORY ${CUBIT_PATH})
        message(FATAL_ERROR "CUBIT_PATH needs to be set to a valid path")
      endif()
  endif()
  set(MESH_ARGS -DCMBNuc_BUILD_MESHKIT:BOOL=${BUILD_MESHKIT}
                -DBUILD_WITH_CUBIT:BOOL=${BUILD_WITH_CUBIT}
                -DCUBIT_PATH:STRING=${CUBIT_PATH})
endif()

if(BUILD_WITH_MOAB)
add_external_project_or_just_build_dependencies(nuclearRGG
  DEPENDS remus qt vtk moab
  CMAKE_ARGS
    ${extra_cmake_args}
    ${MESH_ARGS}
    
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
    ${MESH_ARGS}
    
    # specify the apple app install prefix. No harm in specifying it for all
    # platforms.
    -DMACOSX_APP_INSTALL_PREFIX:PATH=<INSTALL_DIR>/Applications
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DVTK_DIR:PATH=${CMAKE_BINARY_DIR}/vtk/src/vtk-build
)
endif()
