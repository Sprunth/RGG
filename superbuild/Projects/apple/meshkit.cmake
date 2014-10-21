#apple meshkit
#build 64 bit version

option(SUPPRESS_meshkit_BUILD_OUTPUT
"Suppress meshkit build output"
ON)
mark_as_advanced(SUPPRESS_meshkit_BUILD_OUTPUT)

if(SUPPRESS_meshkit_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()


add_external_project(meshkit
  DEPENDS moab cgm lasso
  BUILD_IN_SOURCE 1
  ${suppress_build_out}
  BUILD_COMMAND "make -j5 install"
  INSTALL_COMMAND ""
  PATCH_COMMAND ${GIT_EXECUTABLE} apply ${SuperBuild_PROJECTS_DIR}/patches/meshkit.fix_make_watertight.txt
  CONFIGURE_COMMAND  <SOURCE_DIR>/configure
  --prefix=<INSTALL_DIR>
  --with-itaps=<INSTALL_DIR>
  --enable-algs
  --enable-optimize
  --disable-src
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
    find_path( CUBIT_PATH_ROOT Cubit.app HINTS /Applications/Cubit-13.1 PATH_SUFFIXES "Contents/MacOS/" DOC "The cubit bundle name")
    set(CUBIT_PATH CACHE PATH "Location of the CUBIT Libraries")
    if(CUBIT_PATH_ROOT)
      message("${CUBIT_PATH_ROOT}/Cubit.app/Contents/MacOS/")
      set(CUBIT_PATH "${CUBIT_PATH_ROOT}/Cubit.app/Contents/MacOS/")
      mark_as_advanced(CUBIT_PATH)
    endif()

    if(NOT IS_DIRECTORY ${CUBIT_PATH})
        message(SEND_ERROR "CUBIT_PATH needs to be set to a valid path")
    endif()
  endif()
else()
  set(ENABLE_OCE OFF FORCE)
  mark_as_advanced(ENABLE_OCE)
  set(ENABLE_cgm OFF FORCE)
  mark_as_advanced(ENABLE_cgm)
  set(ENABLE_lasso OFF FORCE)
  mark_as_advanced(ENABLE_lasso)
endif()
