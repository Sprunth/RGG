option(SUPPRESS_meshkit_BUILD_OUTPUT
"Suppress meshkit build output"
ON)
mark_as_advanced(SUPPRESS_meshkit_BUILD_OUTPUT)

if(SUPPRESS_meshkit_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

if(BUILD_WITH_CUBIT)

message("Cubit meshkit")
find_package(Autotools REQUIRED)
add_external_project(meshkit
    BUILD_COMMAND "make -j5 install"
    INSTALL_COMMAND ""
<<<<<<< HEAD
    CMAKE_ARGS   
      -DAUTOHEADER_EXECUTABLE:path=${AUTOHEADER_EXECUTABLE}
      -DAUTOM4TE_EXECUTABLE:path=${AUTOM4TE_EXECUTABLE}
      -DAUTORECONF_EXECUTABLE:path=${AUTORECONF_EXECUTABLE}
      -DAUTOUPDATE_EXECUTABLE:path=${AUTOUPDATE_EXECUTABLE}
=======
    ${suppress_build_out}
    CMAKE_ARGS
>>>>>>> 94deedb5f8ba2b92dafbb1f108a6549094df81ff
      -DBUILD_WITH_CUBIT:BOOL=ON
      -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>/meshkitCubit
      -DCMAKE_PREFIX_PATH:path=<INSTALL_DIR>/meshkitCubit
      -DAUTOHEADER_EXECUTABLE:path=${AUTOHEADER_EXECUTABLE}
      -DAUTOM4TE_EXECUTABLE:path=${AUTOM4TE_EXECUTABLE}
      -DAUTORECONF_EXECUTABLE:path=${AUTORECONF_EXECUTABLE}
      -DAUTOUPDATE_EXECUTABLE:path=${AUTOUPDATE_EXECUTABLE}
      -DCUBIT_PATH:PATH=${CUBIT_PATH})

else()

add_external_project(meshkit
  DEPENDS moab cgm lasso
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ${GIT_EXECUTABLE} apply ${SuperBuild_PROJECTS_DIR}/patches/meshkit.fix_moab_linking_linux.txt
  BUILD_COMMAND "make -j5 install"
  ${suppress_build_out}
  CONFIGURE_COMMAND  <SOURCE_DIR>/configure
  --prefix=<INSTALL_DIR>
  --with-itaps=<INSTALL_DIR>
  --enable-algs
  --enable-optimize
  --enable-src
  --enable-utils
  --enable-rgg
  --enable-shared
  INSTALL_COMMAND ""
)

if(ENABLE_meshkit)
  find_package(Autotools REQUIRED)
  add_external_project_step(meshkit-autoconf
    COMMAND     ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR>
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

endif()

if(ENABLE_meshkit )
  option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )
endif()
