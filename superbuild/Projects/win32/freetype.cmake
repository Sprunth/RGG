add_external_project(
  freetype
  DEPENDS zlib
  CONFIGURE_COMMAND ""
  BUILD_IN_SOURCE 1
  BUILD_COMMAND
    ${CMAKE_COMMAND}
      -DFTJAM_EXECUTABLE:PATH=${FTJAM_EXECUTABLE}
      -DWORKING_DIRECTORY:PATH=<SOURCE_DIR>
      -P ${SuperBuild_PROJECTS_DIR}/win32/freetype-build.cmake
  INSTALL_COMMAND
    ${CMAKE_COMMAND}
      -DSOURCE_DIR:PATH=<SOURCE_DIR>
      -DINSTALL_DIR:PATH=<INSTALL_DIR>
      -P ${SuperBuild_PROJECTS_DIR}/win32/freetype-install.cmake
)

add_extra_cmake_args(
  -DFREETYPE_INCLUDE_DIR_freetype2=<INSTALL_DIR>/include/freetype2
  -DFREETYPE_INCLUDE_DIR_ft2build=<INSTALL_DIR>/include
  -DFREETYPE_LIBRARY=<INSTALL_DIR>/lib/freetype${CMAKE_STATIC_LIBRARY_SUFFIX}
)
