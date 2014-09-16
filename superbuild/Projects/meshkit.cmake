add_external_project(meshkit
  DEPENDS moab cgm lasso
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ${GIT_EXECUTABLE} apply ${SuperBuild_PROJECTS_DIR}/patches/meshkit.fix_moab_linking_linux.txt
  CONFIGURE_COMMAND  <SOURCE_DIR>/configure
  --prefix=<INSTALL_DIR>
  --with-itaps=<INSTALL_DIR>
  --enable-algs
  --enable-optimize
  --enable-src
  --enable-utils
  --enable-rgg
  --enable-shared
)

if(ENABLE_meshkit)
  find_package(Autotools REQUIRED)
  add_external_project_step(meshkit-autoconf
    COMMAND ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR>
    DEPENDEES update
    DEPENDERS configure
  )

  option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )
  if(BUILD_WITH_CUBIT)
    set(CUBIT_PATH CACHE PATH "Location of the CUBIT Libraries")
    if(NOT IS_DIRECTORY ${CUBIT_PATH})
        message(SEND_ERROR "CUBIT_PATH needs to be set to a valid path")
    endif()
  endif()
endif()
