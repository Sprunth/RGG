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
    -DModule_vtkGUISupportQt:bool=ON
    -DModule_vtkGUISupportQtOpenGL:bool=ON
    -DModule_vtkRenderingQt:bool=ON
    -DModule_vtkViewsQt:bool=ON
    -DVTK_REQUIRED_OBJCXX_FLAGS:STRING=""
  ${suppress_build_out}
)
