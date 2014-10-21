option(SUPPRESS_CGM_BUILD_OUTPUT
"Suppress CGM build output"
ON)
mark_as_advanced(SUPPRESS_CGM_BUILD_OUTPUT)

if(SUPPRESS_CGM_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

if(BUILD_WITH_CUBIT)
  add_external_project(cgm
    BUILD_IN_SOURCE 1
    ${suppress_build_out}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --with-cubit=${CUBIT_PATH}
      --prefix=<INSTALL_DIR>
      --enable-shared )
else()
  #we need a external build step since cgm expects occ to be in the system
  #library search path.
  #string(REPLACE " " "\\ " cgm_binary "${CMAKE_CURRENT_BINARY_DIR}/cgm/src/cgm")
  #set(BUILD_STEP ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cgm_build_step.cmake)
  #configure_file(${patch_location}/cgm_build_step.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cgm_build_step.cmake @ONLY)

  #cgm has to be built in source to work
  add_external_project(cgm
    DEPENDS OCE
    BUILD_IN_SOURCE 1
    ${suppress_build_out}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --with-occ=${OCE_DIR}
      --prefix=<INSTALL_DIR>
      --enable-shared
    #BUILD_COMMAND ${BUILD_STEP}
  )
endif()

if(ENABLE_meshkit)
  add_external_project_step(oce-autoconf
    COMMENT "Running autoreconf for oce"
      COMMAND ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR>
      DEPENDEES update
      DEPENDERS configure
  )
endif()
