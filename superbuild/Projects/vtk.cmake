option(SUPPRESS_VTK_BUILD_OUTPUT "Suppress VTK build output" ON)
mark_as_advanced(SUPPRESS_VTK_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_VTK_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(vtk
  DEPENDS qt
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DBUILD_TESTING:BOOL=OFF
    -DModule_vtkGUISupportQt:BOOL=ON
    -DModule_vtkGUISupportQtOpenGL2:BOOL=ON
    -DModule_vtkRenderingQt:BOOL=ON
    -DModule_vtkViewsQt:BOOL=ON
    -DModule_vtkTestingCore:BOOL=ON
    -DModule_vtkTestingRendering:BOOL=ON
    -DVTK_RENDERING_BACKEND:STRING=OpenGL2
    -DVTK_REQUIRED_OBJCXX_FLAGS:STRING=""
  ${suppress_build_out}
)
