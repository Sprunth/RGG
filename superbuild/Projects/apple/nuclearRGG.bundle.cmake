# set extra cpack variables before calling paraview.bundle.common
set (CPACK_GENERATOR DragNDrop)

# include some common stub.
include(nuclearRGG.bundle.common)

#set a root folder inside the package
set(Package_Folder "RGG Suite ${rgg_version_major}.${rgg_version_minor}.${rgg_version_patch}")

include(CPack)

foreach(program ${rgg_programs_to_install})

  install(CODE
  "
  set(PV_PYTHON_LIB_INSTALL_PREFIX
  \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/${program}.app/Contents/Python\")
  "
  COMPONENT superbuild)
  install(CODE "
              file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}\" USE_SOURCE_PERMISSIONS TYPE DIRECTORY FILES
                   \"${install_location}/Applications/${program}.app\")
              file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/${program}.app/Contents/Resources/qt.conf\")
              execute_process(
                COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_bundle.py
                        \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/${program}.app\"
                        \"${install_location}/lib\"
                        \"${install_location}/plugins\")
    "
    COMPONENT superbuild)

  #-----------------------------------------------------------------------------


  #-----------------------------------------------------------------------------
endforeach()

add_test(NAME GenerateRGGPackage
         COMMAND ${CMAKE_CPACK_COMMAND} -G DragNDrop ${test_build_verbose}
         WORKING_DIRECTORY ${SuperBuild_BINARY_DIR})
set_tests_properties(GenerateRGGPackage PROPERTIES
                     LABELS "RGG"
                     TIMEOUT 4800)
