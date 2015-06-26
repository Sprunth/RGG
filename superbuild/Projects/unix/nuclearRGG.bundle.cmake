# script to "bundle" cmb for unix, unlike paraview we are going to dump everything
# into bin instead of having a clear separation of bin and lib.  We have too many
# libraries and executables that have this hardcoded concept that the libraries are
# directly beside the executable

include(nuclearRGG.bundle.common)
include(CPack)

#install(DIRECTORY "@install_location@/lib"
#  DESTINATION "lib"
#  USE_SOURCE_PERMISSIONS
#  COMPONENT superbuild)

install( FILES ${CMAKE_SOURCE_DIR}/../Documentation/UsersGuide/RGGUsersGuide.pdf
DESTINATION "Documentation"
COMPONENT superbuild)

install(FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION bin)

install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/simple_hexflatcore-Modified
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/sixth_hexflatcore
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/sixth_hexvertexcore
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/twelfth_hexflatcore
        ${CMAKE_SOURCE_DIR}/../TestingData/Reactors/doc_rect_example
        DESTINATION ExampleModels)

install(PROGRAMS ${install_location}/lib/RGGNuclear DESTINATION "lib")
install(FILES ${install_location}/lib/materialcolors.ini DESTINATION "lib")


install(CODE
      "execute_process(COMMAND
      ${CMAKE_COMMAND}
        -Dexecutable:PATH=${install_location}/lib/RGGNuclear
        -Ddependencies_root:PATH=${install_location}
        -Dpv_libraries_root:PATH=${install_location}/lib
        -Dcmb_libraries_root:PATH=${install_location}/lib
        -Dtarget_root:PATH=\${CMAKE_INSTALL_PREFIX}/lib
        -DCUBIT_PATH:PATH=${CUBIT_PATH}
        -P ${CMAKE_CURRENT_LIST_DIR}/install_dependencies.cmake)"
    COMPONENT superbuild)

install(PROGRAMS ${install_location}/bin/RGGNuclear DESTINATION "bin")
if(ENABLE_meshkit)
  set(meskit_loc)
  if(BUILD_WITH_CUBIT)
    install(PROGRAMS ${install_location}/lib/RGGNuclear-GUI DESTINATION "lib")
    install(CODE
      "execute_process(COMMAND
      ${CMAKE_COMMAND}
        -Dexecutable:PATH=${install_location}/lib/RGGNuclear-GUI
        -Ddependencies_root:PATH=${install_location}
        -Dpv_libraries_root:PATH=${install_location}/lib
        -Dcmb_libraries_root:PATH=${install_location}/lib
        -Dtarget_root:PATH=\${CMAKE_INSTALL_PREFIX}/lib
        -DCUBIT_PATH:PATH=${CUBIT_PATH}
        -P ${CMAKE_CURRENT_LIST_DIR}/install_dependencies.cmake)"
      COMPONENT superbuild)

    set(meskit_loc ${install_location}/meshkit)

  else()
    set(meskit_loc ${install_location})
  endif()

  install(PROGRAMS ${meskit_loc}/bin/coregen DESTINATION "meshkit")
  install(PROGRAMS ${meskit_loc}/bin/assygen DESTINATION "meshkit")

  install(CODE
      "execute_process(COMMAND
      ${CMAKE_COMMAND}
        -Dexecutable:PATH=${meskit_loc}/bin/coregen
        -Ddependencies_root:PATH=${meskit_loc}
        -Dpv_libraries_root:PATH=${meskit_loc}/lib
        -Dcmb_libraries_root:PATH=${meskit_loc}/lib
        -Dtarget_root:PATH=\${CMAKE_INSTALL_PREFIX}/meshkit
        -DCUBIT_PATH:PATH=${CUBIT_PATH}
        -P ${CMAKE_CURRENT_LIST_DIR}/install_dependencies.cmake)"
    COMPONENT superbuild)

  install(CODE
      "execute_process(COMMAND
      ${CMAKE_COMMAND}
        -Dexecutable:PATH=${meskit_loc}/bin/assygen
        -Ddependencies_root:PATH=${meskit_loc}
        -Dpv_libraries_root:PATH=${meskit_loc}/lib
        -Dcmb_libraries_root:PATH=${meskit_loc}/lib
        -Dtarget_root:PATH=\${CMAKE_INSTALL_PREFIX}/meshkit
        -DCUBIT_PATH:PATH=${CUBIT_PATH}
        -P ${CMAKE_CURRENT_LIST_DIR}/install_dependencies.cmake)"
    COMPONENT superbuild)
endif()

#we have to install everything in bin that also wasn't in lib. The reason for this is that
# all the paraview application in bin are just forward shells
# install executables from bin that aren't in the programs_to install
#install(DIRECTORY
#    "@install_location@/bin/"
#    DESTINATION "bin"
#    USE_SOURCE_PERMISSIONS
#    COMPONENT superbuild
#    FILES_MATCHING
#    REGEX "RGG.*"
#    PATTERN "RGGNuclear")



#install qt
#if (qt_ENABLED AND NOT USE_SYSTEM_qt)
#    install(DIRECTORY
      # install all qt plugins (including sqllite).
      # FIXME: we can reconfigure Qt to be built with inbuilt sqllite support to
      # avoid the need for plugins.
#      "@install_location@/plugins/"
#      DESTINATION "bin"
#      COMPONENT superbuild
#      PATTERN "*.a" EXCLUDE
#      PATTERN "paraview-${pv_version}" EXCLUDE
#      PATTERN "${program}-${cmb_version}" EXCLUDE
#      PATTERN "fontconfig" EXCLUDE
#      PATTERN "*.jar" EXCLUDE
#      PATTERN "*.debug.*" EXCLUDE
#      PATTERN "libboost*" EXCLUDE)
#  endif()

#add the installer as a test
if (BUILD_TESTING)
  add_test(NAME GenerateRGGPackage
           COMMAND ${CMAKE_CPACK_COMMAND} -G TGZ ${test_build_verbose}
           WORKING_DIRECTORY ${SuperBuild_BINARY_DIR})
set_tests_properties(GenerateRGGPackage PROPERTIES
                     LABELS "RGG"
                     TIMEOUT 1200)
endif()
