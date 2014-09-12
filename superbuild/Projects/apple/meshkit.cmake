#apple meshkit
#build 64 bit version

#"CFLAGS=${cflags}" "CXXFLAGS=${cxxflags}"
#set(ldflags "-bind_at_load ${ldflags}")

add_external_project(meshkit
  DEPENDS moab cgm lasso
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ${GIT_EXECUTABLE} apply ${SuperBuild_PROJECTS_DIR}/patches/meshkit.fix_make_watertight.txt
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

add_external_project_step(meshkit-autoconf
    COMMAND autoreconf -i <SOURCE_DIR>
    DEPENDEES update
    DEPENDERS configure
  )
  
if(ENABLE_meshkit)
  option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )
endif()

if(BUILD_WITH_CUBIT)
    set(CUBIT_PATH CACHE PATH "Location of the CUBIT Libraries")
    if(NOT IS_DIRECTORY ${CUBIT_PATH})
        message(FATAL_ERROR "CUBIT_PATH needs to be set to a valid path")
    endif()
endif()
