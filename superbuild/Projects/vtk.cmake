option(SUPPRESS_VTK_BUILD_OUTPUT
       "Suppress VTK build output"
      ON)
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
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SuperBuild_PROJECTS_DIR}/patches/vtk_filters_general_vtkContourTriangulator.cxx
                <SOURCE_DIR>/Filters/General/vtkContourTriangulator.cxx
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DModule_vtkGUISupportQt:bool=ON
    -DModule_vtkGUISupportQtOpenGL:bool=ON
    -DModule_vtkRenderingQt:bool=ON
    -DModule_vtkViewsQt:bool=ON
    -DVTK_REQUIRED_OBJCXX_FLAGS:STRING=""
    ${TMP_QMAKE}
  ${suppress_build_out}
)
