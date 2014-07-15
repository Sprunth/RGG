# script to "bundle" cmb.

#------------------------------------------------------------------------------
# include common stuff.

include(nuclearRGG.bundle.common)
set(CPACK_MONOLITHIC_INSTALL TRUE)

# set NSIS install specific stuff.

# URL to website providing assistance in installing your application.
set (CPACK_NSIS_HELP_LINK "http://paraview.org/Wiki/ParaView")

if(BUILD_DOCUMENTATION)
  set (CPACK_NSIS_MENU_LINKS
    "bin/RGGNuclear.exe" "RGG Nuclear GUI"
    "Documentation/RGGUsersGuide.pdf" "Documentation")
else()
  set (CPACK_NSIS_MENU_LINKS
       "bin/RGGNuclear.exe" "RGG Nuclear GUI")
endif()

#FIXME: need a pretty icon.
#set (CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_LIST_DIR}/paraview.ico")

#------------------------------------------------------------------------------

# install executables to bin. Don't use file glob since that will
# parsed at configure time, and no executables will be installed. Instead use
# install(Directory ) so that it is parsed at install time.
install(DIRECTORY "${install_location}/bin/"
        DESTINATION "bin"
        USE_SOURCE_PERMISSIONS
        COMPONENT superbuild
        REGEX "RGG.*"
        PATTERN "CMBNuclear")

# install all dlls to bin. This will install all VTK/ParaView dlls plus any
# other tool dlls that were placed in bin.
install(DIRECTORY "${install_location}/bin/"
        DESTINATION "bin"
        USE_SOURCE_PERMISSIONS
        COMPONENT superbuild
        FILES_MATCHING PATTERN "*.dll")

if (qt_ENABLED AND NOT USE_SYSTEM_qt)
  install(DIRECTORY
    # install all qt plugins (including sqllite).
    # FIXME: we can reconfigure Qt to be built with inbuilt sqllite support to
    # avoid the need for plugins.
    "${install_location}/plugins/"
    DESTINATION "bin"
    COMPONENT ParaView
    PATTERN "*.dll")
endif()

if(BUILD_DOCUMENTATION)
  install(FILES ${install_location}/Docs/RGGUsersGuide.pdf
          DESTINATION "Documentation"
          COMPONENT superbuild)
endif()

# install system runtimes.
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "bin")
include(InstallRequiredSystemLibraries)

#-----------------------------------------------------------------------------
# include CPack at end so that all COMPONENTs specified in install rules are
# correctly detected.
include(CPack)

add_test(NAME GenerateRGGPackage-NSIS
         COMMAND ${CMAKE_CPACK_COMMAND} -G NSIS ${test_build_verbose}
         WORKING_DIRECTORY ${SuperBuild_BINARY_DIR})

add_test(NAME GenerateRGGPackage-ZIP
         COMMAND ${CMAKE_CPACK_COMMAND} -G ZIP ${test_build_verbose}
         WORKING_DIRECTORY ${SuperBuild_BINARY_DIR})

set_tests_properties(GenerateRGGPackage-NSIS
                     GenerateRGGPackage-ZIP
                     PROPERTIES
                     # needed so that tests are run on typical paraview
                     # dashboards
                     LABELS "RGG"
                     TIMEOUT 1200) # increase timeout to 20 mins.
