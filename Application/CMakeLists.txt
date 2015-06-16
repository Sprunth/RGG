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
  cmbNucMainWindow.cxx
  cmbNucAboutDialog.cxx
  cmbNucAssembly.cxx
  cmbNucAssemblyLink.cxx
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
  cmbNucLattice.cxx
  cmbNucLatticeWidget.cxx
  vtkCmbLayeredConeSource.cxx
  cmbNucPartsTreeItem.cxx
  cmbNucMaterialTreeItem.cxx
  cmbNucPinLibrary.cxx
  cmbNucPinCell.cxx
  cmbNucPinCellEditor.cxx
  cmbNucDuctCell.cxx
  cmbNucDuctLibrary.cxx
  cmbNucDraw2DLattice.cxx
  DrawLatticeItem.cxx
  cmbNucExport.cxx
  cmbNucExportDialog.cxx
  cmbNucPreferencesDialog.cxx
  cmbAssyParametersWidget.cxx
  inpFileIO.cxx
  xmlFileIO.cxx
  cmbCoreParametersWidget.cxx
  pqXMLEventObserver.cxx
  cmbNucRender.cxx
  cmbNucInpExporter.cxx
  cmbNucImporter.cxx
)

SET(RGGNuclear_UI
  qNucMainWindow.ui
  qAboutDialog.ui
  qInputListWidget.ui
  qInputPropertiesWidget.ui
  cmbNucPinCellEditor.ui
  cmbDuctCellEditor.ui
  qExporterDialog.ui
  qProgress.ui
  qCoregenModel.ui
  qDefaults.ui
  Preferences.ui
  qAssyParameters.ui
  qCoreParameters.ui
  qImportLog.ui
)

SET(RGGNuclear_Headers
  cmbNucAboutDialog.h
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
  cmbNucPinLibrary.h
  cmbNucPinCell.h
  cmbNucPinCellEditor.h
  cmbNucDuctCell.h
  cmbNucDuctLibrary.h
  cmbNucDraw2DLattice.h
  cmbNucExport.h
  cmbNucExportDialog.h
  cmbNucPreferencesDialog.h
  cmbAssyParametersWidget.h
  cmbCoreParametersWidget.h
  pqXMLEventObserver.h
  cmbNucRender.h
)

SET(RGGNuclear_other_headers )

FIND_PACKAGE(MOAB)
message(${MOAB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${MOAB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(moab_wrapper)
SET(RGGNuclear_Headers ${RGGNuclear_Headers} cmbNucCoregen.h)
SET(RGGNuclear_other_headers ${RGGNuclear_other_headers}
                             moab_wrapper/CellSets.h
                             moab_wrapper/DataSetConverter.h
                             moab_wrapper/detail/CellTypeToType.h
                             moab_wrapper/detail/ContinousCellInfo.h
                             moab_wrapper/detail/LoadGeometry.h
                             moab_wrapper/detail/MixedCellConnectivity.h
                             moab_wrapper/detail/ReadSparseTag.h
                             moab_wrapper/detail/ReduceSpectralMesh.h
                             moab_wrapper/detail/UsageTable.h
                             moab_wrapper/ExtractShell.h
                             moab_wrapper/FaceSets.h
                             moab_wrapper/SimpleMoab.h
                             moab_wrapper/vtkMoabReader.h
                             moab_wrapper/extract_subset.h)
SET(RGGNuclear_Srcs ${RGGNuclear_Srcs}
                    moab_wrapper/vtkMoabReader.cxx
                    moab_wrapper/extract_subset.cxx
                    cmbNucCoregen.cxx )
SET(MOAB_LIB MOAB)
SET_SOURCE_FILES_PROPERTIES(cmbNucMainWindow.cxx cmbNucMainWindow.h
                            PROPERTIES
                            COMPILE_FLAGS "-DBUILD_WITH_MOAB")

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

set(TARGET_NAME)

find_package(Git)

execute_process (COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                 OUTPUT_VARIABLE BUILD_ID)

add_compile_options( -DRGG_VERSION=${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                     -DRGG_BUILD_ID=${BUILD_ID} )

if(WIN32 OR APPLE)
  ADD_EXECUTABLE( RGGNuclear MACOSX_BUNDLE cmbNucMain.cxx ${RGGNuclear_Srcs} ${RGGNuclear_other_headers} ${UISrcs} ${MOCSrcs} ${RCS_SOURCES} ${ICON_FILE})
  TARGET_LINK_LIBRARIES( RGGNuclear RemusClient RemusWorker RemusServer ${MOAB_LIB} ${QT_LIBRARIES} ${VTK_LIBRARIES} ${QtTesting_LIBRARIES})
else()
  include(vtkForwardingExecutable)
  set(VTK_INSTALL_LIBRARY_DIR "lib")
  set(BUILD_SHARED_LIBS TRUE)

  set(TARGET_NAME RGGNuclear)
  vtk_add_executable_with_forwarding(rgg_exe_suffix RGGNuclear cmbNucMain.cxx ${RGGNuclear_Srcs} ${RGGNuclear_other_headers}
                                                    ${UISrcs} ${MOCSrcs} ${RCS_SOURCES} ${ICON_FILE})
  TARGET_LINK_LIBRARIES(RGGNuclear RemusClient RemusWorker RemusServer ${MOAB_LIB} ${QT_LIBRARIES} ${VTK_LIBRARIES} ${QtTesting_LIBRARIES})

  IF (rgg_exe_suffix)
     # Shared forwarding enabled.
     INSTALL(TARGETS RGGNuclear${rgg_exe_suffix}
             DESTINATION "bin"
             COMPONENT Runtime)
  ENDIF()
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
  install(TARGETS ${TARGET_NAME} DESTINATION lib)
endif()

if(BUILD_TESTING)
  ENABLE_TESTING()
  INCLUDE(CTest)
  if(Q_WS_MAC)
    set(TEST_BINARY "${EXECUTABLE_OUTPUT_PATH}/RGGNuclear.app/Contents/MacOS/RGGNuclear"
                    --test-directory=${CMAKE_SOURCE_DIR}/TestingData)
  else(Q_WS_MAC)
    set(TEST_BINARY "${EXECUTABLE_OUTPUT_PATH}/RGGNuclear"
                    --test-directory=${CMAKE_SOURCE_DIR}/TestingData)
  endif(Q_WS_MAC)

  set (TESTS_BOTH_2D_3D  ImportHexFull
                         ImportHexSixthVert
                         ImportRect
                         OpenHexSixthFlat
                         OpenHexTwelth
                         ImportHexSixthFlat
                         ImportHexTwelth
                         OpenHexFull
                         OpenHexSixthVert
                         OpenRect )

  set (TESTS_WITH_MODEL_BASELINES ${TESTS_BOTH_2D_3D})

  set (TESTS_WITH_MESH_BASELINES )

  set (LONG_TEST )

  set (TESTS_WITH_2D_BASELINES  ${TESTS_BOTH_2D_3D})

  set (TESTS_WITHOUT_BASELINES exit TestAbout)

  foreach(test ${TESTS_WITHOUT_BASELINES})
    add_test(NAME ${test}_baseline
             COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}
                                    --exit  --test-output-directory=${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${test}_baseline PROPERTIES TIMEOUT 1000)
    if(APPLE)
      set_tests_properties(${test}_baseline PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:$ENV{DYLD_LIBRARY_PATH}")
    elseif(UNIX)
      if(BUILD_WITH_CUBIT)
        set_tests_properties(${test}_baseline PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:${CUBIT_PATH}/bin")
      else()
        set_tests_properties(${test}_baseline PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
      endif()
    endif()
  endforeach()

  foreach(test ${TESTS_WITH_2D_BASELINES})
    add_test(NAME ${test}_2D
             COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}.xml
                                    --exit
                                    --test-output-directory=${CMAKE_CURRENT_BINARY_DIR}
                                    --truth-2D-images=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_2D.png)
    set_tests_properties(${test}_2D PROPERTIES TIMEOUT 1000)
    if(APPLE)
      set_tests_properties(${test}_2D PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:$ENV{DYLD_LIBRARY_PATH}")
    elseif(UNIX)
      if(BUILD_WITH_CUBIT)
        set_tests_properties(${test}_2D PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:${CUBIT_PATH}/bin")
      else()
        set_tests_properties(${test}_2D PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
      endif()
    endif()
  endforeach()

  foreach(test ${TESTS_WITH_MODEL_BASELINES})
    add_test(NAME ${test}_model
             COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}.xml
                                    --exit
                                    --test-output-directory=${CMAKE_CURRENT_BINARY_DIR}
                                    --truth-model-images=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_model.png)
    set_tests_properties(${test}_model PROPERTIES TIMEOUT 1000)
    if(APPLE)
      set_tests_properties(${test}_model PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:$ENV{DYLD_LIBRARY_PATH}")
    elseif(UNIX)
      if(BUILD_WITH_CUBIT)
        set_tests_properties(${test}_model PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:${CUBIT_PATH}/bin")
      else()
        set_tests_properties(${test}_model PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
      endif()
    endif()
  endforeach()

  foreach(test ${TESTS_WITH_MESH_BASELINES})
    add_test(NAME ${test}_mesh
             COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}.xml
                                    --exit
                                    --test-output-directory=${CMAKE_CURRENT_BINARY_DIR}
                                    --truth-mesh-image=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_mesh.png)
    set_tests_properties(${test}_mesh PROPERTIES TIMEOUT 1000)
    if(APPLE)
      set_tests_properties(${test}_mesh PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:$ENV{DYLD_LIBRARY_PATH}")
    elseif(UNIX)
      if(BUILD_WITH_CUBIT)
        set_tests_properties(${test}_mesh PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:${CUBIT_PATH}/bin")
      else()
        set_tests_properties(${test}_mesh PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
      endif()
    endif()
  endforeach()

  foreach(test ${LONG_TEST})
    add_test(NAME ${test}_long
             COMMAND ${TEST_BINARY} --test-script=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}.xml
                                    --exit
                                    --test-output-directory=${CMAKE_CURRENT_BINARY_DIR}
                                    --truth-model-images=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_long_3D.png
                                    --truth-2D-images=${CMAKE_CURRENT_SOURCE_DIR}/Testing/XML/${test}_long_2D.png)
    set_tests_properties(${test}_long PROPERTIES TIMEOUT 3200)
    if(APPLE)
      set_tests_properties(${test}_long PROPERTIES ENVIRONMENT "DYLD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:$ENV{DYLD_LIBRARY_PATH}")
    elseif(UNIX)
      if(BUILD_WITH_CUBIT)
        set_tests_properties(${test}_long PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib:${CUBIT_PATH}/bin")
      else()
        set_tests_properties(${test}_long PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${MOAB_ROOT_DIR}/lib")
      endif()
    endif()
  endforeach()
endif()
