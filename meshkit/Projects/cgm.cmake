if(BUILD_WITH_CUBIT)
  set(ldFlags "")
  if(APPLE)
    set(ldFlags "LDFLAGS=-headerpad_max_install_names -L${CUBIT_PATH}")
  else()
    set(ldFlags "LDFLAGS=-L${CUBIT_PATH}")
  endif()
  message("CGM: CFLAGS=${cflags} CXXFLAGS=${cxxflags}")
  add_external_project(cgm
    USE_AUTOCONF
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --with-cubit=${CUBIT_PATH}
      --prefix=<INSTALL_DIR>
      --enable-shared
      ${ldFlags}
      "CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
    BUILD_COMMAND ${BUILD_STEP} )
else()

  get_libraries(OCE oce_libraries)

  get_filename_component(oce_path ${oce_libraries} PATH)


  #we need a external build step since cgm expects occ to be in the system
  #library search path.
  string(REPLACE " " "\\ " cgm_binary "${CMAKE_CURRENT_BINARY_DIR}/cgm/src/cgm")
  set(BUILD_STEP ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cgm_build_step.cmake)
  configure_file(${patch_location}/cgm_build_step.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cgm_build_step.cmake @ONLY)

  #cgm has to be built in source to work
  add_external_project(cgm
    DEPENDS OCE
    USE_AUTOCONF
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --with-occ=<INSTALL_DIR>
      --prefix=<INSTALL_DIR>
      --enable-shared
      "CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
    BUILD_COMMAND ${BUILD_STEP})

endif()
