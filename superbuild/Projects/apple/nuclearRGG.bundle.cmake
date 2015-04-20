# set extra cpack variables before calling paraview.bundle.common
set (CPACK_GENERATOR DragNDrop)

# include some common stub.
include(nuclearRGG.bundle.common)

#set a root folder inside the package
set(Package_Folder "RGG_Suite_${rgg_version_major}.${rgg_version_minor}.${rgg_version_patch}")

include(CPack)

set(SEARCH_LOC ${install_location}/lib)
if(${USE_SYSTEM_OCE})
  set(SEARCH_LOC "${install_location}/lib;${OCE_DIR}/lib")
endif()

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
                        \"${SEARCH_LOC}\"
                        \"${install_location}/plugins\")
    "
    COMPONENT superbuild)

  #-----------------------------------------------------------------------------


  #-----------------------------------------------------------------------------
endforeach()

if(ENABLE_meshkit)
  #set(rgg_programs_to_install ${rgg_programs_to_install} coregen)
  install(CODE "
              file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/coregen/Contents/bin\" USE_SOURCE_PERMISSIONS TYPE DIRECTORY FILES
                   \"${install_location}/bin/coregen\")
              execute_process(
                COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_bundle.py
                        \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/coregen\"
                        \"${SEARCH_LOC}\"
                        \"${install_location}/plugins\")
    "
    COMPONENT superbuild)
  install(CODE "
               file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/postbl/Contents/bin\" USE_SOURCE_PERMISSIONS TYPE DIRECTORY FILES
                  \"${install_location}/bin/PostBL\")
               execute_process( COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_bundle.py
                                \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/postbl\"
                                \"${SEARCH_LOC}\"
                                \"${install_location}/plugins\")
               "
          COMPONENT superbuild)
  if(BUILD_WITH_CUBIT)
    install(CODE "
              file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/assygen/Contents/bin\" USE_SOURCE_PERMISSIONS TYPE DIRECTORY FILES
                   \"${install_location}/32bit/meshkit/bin/assygen\")
              execute_process(
                COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_bundle.py
                        \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/assygen\"
                        \"${install_location}/32bit/meshkit/lib\"
                        \"${install_location}/plugins\")
    "
    COMPONENT superbuild)
    install(CODE "
              file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/assygen/Contents/bin\" USE_SOURCE_PERMISSIONS TYPE DIRECTORY FILES
                   \"${install_location}/32bit/meshkit/bin/coregen\")
              execute_process(
                COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_bundle.py
                        \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/assygen\"
                        \"${install_location}/32bit/meshkit/lib\"
                        \"${install_location}/plugins\")
    "
    COMPONENT superbuild)
  else()
    install(CODE "
              file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/assygen/Contents/bin\" USE_SOURCE_PERMISSIONS TYPE DIRECTORY FILES
                   \"${install_location}/bin/assygen\")
              execute_process(
                COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_bundle.py
                        \"\${CMAKE_INSTALL_PREFIX}/${Package_Folder}/RGGNuclear.app/meshkit/assygen\"
                        \"${install_location}/lib\"
                        \"${install_location}/plugins\")
    "
    COMPONENT superbuild)
    #TODO: need better support for cylinder generation with occ.  Right now not being supported
  endif()
endif()


if(BUILD_DOCUMENTATION)
  install(FILES ${install_location}/Docs/RGGUsersGuide.pdf
          DESTINATION "Documentation"
          COMPONENT superbuild)
endif()

install(DIRECTORY 
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/simple_hexflatcore-Modified
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/sixth_hexflatcore
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/sixth_hexvertexcore
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/twelfth_hexflatcore
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/doc_rect_example
        DESTINATION ExampleModels)

add_test(NAME GenerateRGGPackage
         COMMAND ${CMAKE_CPACK_COMMAND} -G DragNDrop ${test_build_verbose}
         WORKING_DIRECTORY ${SuperBuild_BINARY_DIR})
set_tests_properties(GenerateRGGPackage PROPERTIES
                     LABELS "RGG"
                     TIMEOUT 4800)
