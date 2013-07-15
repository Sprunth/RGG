PROJECT(CMBNuclear)

FIND_PACKAGE(VTK)
INCLUDE(${VTK_USE_FILE}) # include UseVTK.cmake

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})  # include UseQt4.cmake

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
  cmbNucDuctSource.cxx
  cmbNucInputListWidget.cxx
  cmbNucInputPropertiesWidget.cxx
  cmbNucPartDefinition.h
  vtkCmbConeSource.cxx
  cmbNucPartsTreeItem.cxx
  cmbNucDragLabel.cxx
)

SET(CMBNuclear_UI
  qNucMainWindow.ui
  qInputListWidget.ui
  qInputPropertiesWidget.ui)

SET(CMBNuclear_Headers
  cmbNucMainWindow.h
  cmbNucInputListWidget.h
  cmbNucInputPropertiesWidget.h
  cmbNucPartsTreeItem.h
  cmbNucAssemblyEditor.h
  cmbNucDragLabel.h
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
TARGET_LINK_LIBRARIES( CMBNuclear ${QT_LIBRARIES} ${VTK_LIBRARIES} )
