PROJECT(CMBNuclear)

FIND_PACKAGE(VTK)
INCLUDE(${VTK_USE_FILE}) # include UseVTK.cmake

FIND_PACKAGE(Qt4 REQUIRED QtCore QtGui)
INCLUDE(${QT_USE_FILE})  # include UseQt4.cmake

FIND_PACKAGE(Remus REQUIRED)
INCLUDE(${Remus_INCLUDE_DIR})
message("${Remus_INCLUDE_DIR}")

# support for out-of-source build
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR} #this is where ui_qNucMainWindow.h is generated
  ${CMAKE_CURRENT_SOURCE_DIR}
)

#Now the QtVTK version
SET(CMBNuclear_Srcs
  cmbNucMain.cxx
  cmbNucMainWindow.cxx
  cmbNucAssembly.cxx
  cmbNucAssemblyEditor.cxx
  cmbNucCore.cxx
  cmbNucInputListWidget.cxx
  cmbNucInputPropertiesWidget.cxx
  cmbNucNewDialog.cxx
  cmbNucMaterialColors.cxx
  cmbNucPartDefinition.h
  vtkCmbConeSource.cxx
  vtkCmbDuctSource.cxx
  vtkCmbLayeredConeSource.cxx
  cmbNucPartsTreeItem.cxx
  cmbNucDragLabel.cxx
  cmbNucPinCellEditor.cxx
  cmbNucHexLattice.cxx
  HexLatticeItem.cxx
  cmbNucExport.cxx
  cmbNucExportDialog.cxx
  cmbNucPreferencesDialog.cxx
)

SET(CMBNuclear_UI
  qNucMainWindow.ui
  qNucNewDialog.ui
  qInputListWidget.ui
  qInputPropertiesWidget.ui
  cmbNucPinCellEditor.ui
  qExporterDialog.ui
  qProgress.ui
  Preferences.ui
)

SET(CMBNuclear_Headers
  cmbNucMainWindow.h
  cmbNucInputListWidget.h
  cmbNucInputPropertiesWidget.h
  cmbNucNewDialog.h
#  cmbNucPartsTreeItem.h
  cmbNucAssemblyEditor.h
  cmbNucDragLabel.h
  cmbNucPinCellEditor.h
  cmbNucHexLattice.h
  HexLatticeItem.h
  cmbNucExport.h
  cmbNucExportDialog.h
  cmbNucPreferencesDialog.h
)

QT4_WRAP_UI(UISrcs ${CMBNuclear_UI})
QT4_WRAP_CPP(MOCSrcs ${CMBNuclear_Headers} )
QT4_ADD_RESOURCES(RCS_SOURCES
  qNucResources.qrc)
SOURCE_GROUP("Resources" FILES
  ${CMBNuclear_UI} ${RCS_SOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
)

ADD_EXECUTABLE( CMBNuclear MACOSX_BUNDLE ${CMBNuclear_Srcs} ${UISrcs} ${MOCSrcs} ${RCS_SOURCES})
TARGET_LINK_LIBRARIES( CMBNuclear RemusClient RemusWorker RemusServer ${QT_LIBRARIES} ${VTK_LIBRARIES} )

# The initial material color file
set (materialColorsName "materialcolors")
if(WIN32)
  #quick work around since we don't know what build type we are doing on windows
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/materialcolors.ini.in"
             ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini
             @ONLY)

  add_custom_command(OUTPUT ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${materialColorsName}.ini
    COMMAND ${CMAKE_COMMAND} -E copy
    ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini
    ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${materialColorsName}.ini
    )

  add_custom_target(Copy${materialColorsName}WorkerRegFile ALL DEPENDS
    ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${materialColorsName}.ini)

  install (FILES ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini DESTINATION bin)
elseif(APPLE)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/materialcolors.ini.in"
             ${EXECUTABLE_OUTPUT_PATH}/CMBNuclear.app/Contents/MacOS/${materialColorsName}.ini
             @ONLY)
  #on the mac the marterialcolor file needs to be installed into the CMBNuclear app
  install (FILES ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini DESTINATION
          ${CMAKE_INSTALL_PREFIX}/CMBNuclear.app/Contents/MacOS)
else()
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/materialcolors.ini.in"
             ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini
             @ONLY)
  install (FILES ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini DESTINATION bin)
endif()
