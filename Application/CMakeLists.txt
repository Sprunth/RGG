PROJECT(RGGNuclear)

FIND_PACKAGE(VTK)
INCLUDE(${VTK_USE_FILE}) # include UseVTK.cmake

find_package(QtTesting REQUIRED)
INCLUDE_DIRECTORIES(${QtTesting_INCLUDE_DIRS})

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})  # include UseQt4.cmake
INCLUDE_DIRECTORIES(${QT_INCLUDE_DIR})

FIND_PACKAGE(Remus REQUIRED)
INCLUDE_DIRECTORIES(${Remus_INCLUDE_DIR})
message("${Remus_INCLUDE_DIR}")

# support for out-of-source build
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR} #this is where ui_qNucMainWindow.h is generated
  ${CMAKE_CURRENT_SOURCE_DIR}
)

#Now the QtVTK version
SET(RGGNuclear_Srcs
  cmbNucMain.cxx
  cmbNucMainWindow.cxx
  cmbNucAssembly.cxx
  cmbNucCore.cxx
  cmbNucDuctCellEditor.cxx
  cmbNucDefaults.cxx
  cmbNucDefaultWidget.cxx
  cmbNucGenerateOuterCylinder.cxx
  cmbNucInputListWidget.cxx
  cmbNucInputPropertiesWidget.cxx
  cmbNucMaterial.cxx
  cmbNucMaterialColors.cxx
  cmbNucPartDefinition.h
  vtkCmbDuctSource.cxx
  cmbNucLattice.cxx
  cmbNucLatticeWidget.cxx
  vtkCmbLayeredConeSource.cxx
  cmbNucPartsTreeItem.cxx
  cmbNucMaterialTreeItem.cxx
  cmbNucPinCell.cxx
  cmbNucPinCellEditor.cxx
  cmbNucDuctCell.cxx
  cmbNucDraw2DLattice.cxx
  DrawLatticeItem.cxx
  cmbNucExport.cxx
  cmbNucExportDialog.cxx
  cmbNucPreferencesDialog.cxx
  cmbAssyParametersWidget.cxx
  inpFileIO.cxx
  cmbCoreParametersWidget.cxx
  pqXMLEventObserver.cxx
)

SET(RGGNuclear_UI
  qNucMainWindow.ui
  qInputListWidget.ui
  qInputPropertiesWidget.ui
  cmbNucPinCellEditor.ui
  cmbDuctCellEditor.ui
  qExporterDialog.ui
  qProgress.ui
  qCoregenModel.ui
  qDefaults.ui
  Preferences.ui
  CylinderCreateGui.ui
  qAssyParameters.ui
  qCoreParameters.ui
)

SET(RGGNuclear_Headers
  cmbNucAssembly.h
  cmbNucCore.h
  cmbNucDuctCellEditor.h
  cmbNucDefaults.h
  cmbNucDefaultWidget.h
  cmbNucMainWindow.h
  cmbNucLatticeWidget.h
  cmbNucInputListWidget.h
  cmbNucInputPropertiesWidget.h
  cmbNucMaterial.h
  cmbNucMaterialColors.h
  cmbNucPartsTreeItem.h
  cmbNucMaterialTreeItem.h
  cmbNucPinCell.h
  cmbNucPinCellEditor.h
  cmbNucDuctCell.h
  cmbNucDraw2DLattice.h
  DrawLatticeItem.h
  cmbNucExport.h
  cmbNucExportDialog.h
  cmbNucPreferencesDialog.h
  cmbAssyParametersWidget.h
  cmbCoreParametersWidget.h
  pqXMLEventObserver.h
)

SET(RGGNuclear_other_headers )

OPTION( VTK_MOAB_SUPPORT "Build with VTK MOAB support"  OFF )
IF(VTK_MOAB_SUPPORT)
  FIND_PACKAGE(MOAB)
  message(${MOAB_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${MOAB_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(vtk_moab_reader)
  SET(RGGNuclear_Headers ${RGGNuclear_Headers} cmbNucCoregen.h)
  SET(RGGNuclear_other_headers ${RGGNuclear_other_headers}
                               vtk_moab_reader/CellSets.h
                               vtk_moab_reader/DataSetConverter.h
                               vtk_moab_reader/detail/CellTypeToType.h
                               vtk_moab_reader/detail/ContinousCellInfo.h
                               vtk_moab_reader/detail/LoadGeometry.h
                               vtk_moab_reader/detail/MixedCellConnectivity.h
                               vtk_moab_reader/detail/ReadSparseTag.h
                               vtk_moab_reader/detail/ReduceSpectralMesh.h
                               vtk_moab_reader/detail/UsageTable.h
                               vtk_moab_reader/ExtractShell.h
                               vtk_moab_reader/FaceSets.h
                               vtk_moab_reader/SimpleMoab.h
                               vtk_moab_reader/vtkMoabReader.h)
  SET(RGGNuclear_Srcs ${RGGNuclear_Srcs}
                      vtk_moab_reader/vtkMoabReader.cxx
                      cmbNucCoregen.cxx )
  SET(MOAB_LIB MOAB)
  SET_SOURCE_FILES_PROPERTIES(cmbNucMainWindow.cxx cmbNucMainWindow.h
                              PROPERTIES
                              COMPILE_FLAGS "-DBUILD_WITH_MOAB")
ENDIF()

QT4_WRAP_UI(UISrcs ${RGGNuclear_UI})
QT4_WRAP_CPP(MOCSrcs ${RGGNuclear_Headers} )
QT4_ADD_RESOURCES(RCS_SOURCES
  qNucResources.qrc)
SOURCE_GROUP("Resources" FILES
  ${RGGNuclear_UI} ${RCS_SOURCES}
)

SOURCE_GROUP("Generated" FILES
  ${UISrcs}
  ${MOCSrcs}
)

if (WIN32)
  set( ICON_FILE RGGNuclear.rc )
endif()

if(APPLE)
  set( ICON_FILE MacIcon.icns )
  SET( MACOSX_BUNDLE_INFO_STRING "RGGNuclear" )
  SET( MACOSX_BUNDLE_ICON_FILE MacIcon.icns )
  SET( MACOSX_BUNDLE_BUNDLE_NAME "RGGNuclear" )
  SET_SOURCE_FILES_PROPERTIES( MacIcon.icns PROPERTIES
                               MACOSX_PACKAGE_LOCATION
                               Resources )
endif()

if(WIN32 OR APPLE)
ADD_EXECUTABLE( RGGNuclear MACOSX_BUNDLE ${RGGNuclear_Srcs} ${RGGNuclear_other_headers} ${UISrcs} ${MOCSrcs} ${RCS_SOURCES} ${ICON_FILE})
TARGET_LINK_LIBRARIES( RGGNuclear RemusClient RemusWorker RemusServer ${MOAB_LIB} ${QT_LIBRARIES} ${VTK_LIBRARIES} ${QtTesting_LIBRARIES})
else()

message ("Forwarding")
  include(vtkForwardingExecutable)
  set(VTK_INSTALL_LIBRARY_DIR "lib")
  set(BUILD_SHARED_LIBS TRUE)
  vtk_add_executable_with_forwarding(rgg_exe_suffix
                 RGGNuclear
                 ${RGGNuclear_Srcs} ${RGGNuclear_other_headers} ${UISrcs} ${MOCSrcs} ${RCS_SOURCES} ${ICON_FILE}
                 )
  TARGET_LINK_LIBRARIES(RGGNuclear RemusClient RemusWorker RemusServer ${MOAB_LIB} ${QT_LIBRARIES} ${VTK_LIBRARIES} ${QtTesting_LIBRARIES})

  IF (rgg_exe_suffix)
     # Shared forwarding enabled.
     INSTALL(TARGETS RGGNuclear${rgg_exe_suffix}
             DESTINATION "bin"
             COMPONENT Runtime)
  ENDIF ()
endif()


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
  install(TARGETS RGGNuclear DESTINATION bin)
elseif(APPLE)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/materialcolors.ini.in"
             ${EXECUTABLE_OUTPUT_PATH}/RGGNuclear.app/Contents/MacOS/${materialColorsName}.ini
             @ONLY)
  #on the mac the marterialcolor file needs to be installed into the RGGNuclear app
  install (FILES ${EXECUTABLE_OUTPUT_PATH}/RGGNuclear.app/Contents/MacOS/${materialColorsName}.ini DESTINATION
          ${CMAKE_INSTALL_PREFIX}/Applications/RGGNuclear.app/Contents/MacOS)
  install(TARGETS RGGNuclear DESTINATION ${CMAKE_INSTALL_PREFIX}/Applications)
else()
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/materialcolors.ini.in"
             ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini
             @ONLY)
  install (FILES ${EXECUTABLE_OUTPUT_PATH}/${materialColorsName}.ini DESTINATION lib)
  install(TARGETS RGGNuclear DESTI NATION lib)
endif()

if(BUILD_TESTING)
  if(Q_WS_MAC)
    set(TEST_BINARY
      "${EXECUTABLE_OUTPUT_PATH}/RGGNuclear.app/Contents/MacOS/RGGNuclear"
      --test-directory=${CMAKE_SOURCE_DIR}/TestingData)
  else(Q_WS_MAC)
    set(TEST_BINARY "${EXECUTABLE_OUTPUT_PATH}/RGGNuclear"
      --test-directory=${CMAKE_SOURCE_DIR}/TestingData)
  endif(Q_WS_MAC)

  set (TESTS_WITH_MODEL_BASELINES  simple_rect_test
                                   simple_rect_assy_test
                                   simple_hex_test
                                   simple_hex_assy_test
                                   test_open_12th_hex
                                   test_open_6th_flat
                                   test_open_6th_vertex
                                   test_open_assy_hex
                                   test_open_assy_rect
                                   test_new_rect
                                   test_new_hex
    )

  set (TESTS_WITH_2D_BASELINES  simple_rect_test
                                simple_rect_assy_test
                                simple_hex_test
                                simple_hex_assy_test
                                test_open_12th_hex
                                test_open_6th_flat
                                test_open_6th_vertex
                                test_open_assy_hex
                                test_open_assy_rect
                                test_new_rect
                                test_new_hex
    )

  set (TESTS_WITHOUT_BASELINES exit
    )
    
  foreach(test ${TESTS_WITHOUT_BASELINES})
    add_test(NAME ${test}_baseline COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test} --exit  --test-output-directory=${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${test}_baseline PROPERTIES TIMEOUT 10000)
    if(APPLE)
      set_tests_properties(${test}_baseline PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
    endif()
  endforeach()
  
  foreach(test ${TESTS_WITH_2D_BASELINES})
    add_test(NAME ${test}_2D COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}.xml --exit  --test-output-directory=${CMAKE_CURRENT_BINARY_DIR} --truth-2D-images=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_2D.png)
    set_tests_properties(${test}_2D PROPERTIES TIMEOUT 10000)
    if(APPLE)
      set_tests_properties(${test}_2D PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
    endif()
  endforeach()
  
  foreach(test ${TESTS_WITH_MODEL_BASELINES})
    add_test(NAME ${test}_model COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}.xml --exit  --test-output-directory=${CMAKE_CURRENT_BINARY_DIR} --truth-model-images=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_model.png)
    set_tests_properties(${test}_model PROPERTIES TIMEOUT 10000)
    if(APPLE)
      set_tests_properties(${test}_model PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
    endif()
  endforeach()

endif()
