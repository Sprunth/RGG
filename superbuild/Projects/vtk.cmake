option(SUPPRESS_VTK_BUILD_OUTPUT
       "Suppress VTK build output"
      ON)
mark_as_advanced(SUPPRESS_VTK_BUILD_OUTPUT)

set(suppress_build_out)

if(SUPPRESS_VTK_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(vtk
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DVTK_Group_Qt:bool=ON
  ${suppress_build_out}
)
