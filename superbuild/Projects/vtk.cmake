option(SUPPRESS_VTK_BUILD_OUTPUT "Suppress VTK build output" ON)
mark_as_advanced(SUPPRESS_VTK_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_VTK_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

#if(${USE_SYSTEM_qt})
#set(TMP_QMAKE -DQT_QMAKE_EXECUTABLE:PATH="${QT_QMAKE_EXECUTABLE}")
#endif()

add_external_project(vtk
  DEPENDS qt
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DModule_vtkGUISupportQt:bool=ON
    -DModule_vtkGUISupportQtOpenGL2:bool=ON
    -DModule_vtkRenderingQt:bool=ON
    -DModule_vtkViewsQt:bool=ON
    -DVTK_RENDERING_BACKEND:STRING=OpenGL2
    -DVTK_REQUIRED_OBJCXX_FLAGS:STRING=""
    ${TMP_QMAKE}
  ${suppress_build_out}
)
