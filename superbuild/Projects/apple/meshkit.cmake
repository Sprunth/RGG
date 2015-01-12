#apple meshkit
#build 64 bit version

option(SUPPRESS_meshkit_BUILD_OUTPUT
"Suppress meshkit build output"
ON)
mark_as_advanced(SUPPRESS_meshkit_BUILD_OUTPUT)

if(SUPPRESS_meshkit_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
set(junk_output "&> /dev/null")
endif()

if(BUILD_MESHKIT_MASTER)
  add_external_project(meshkit
    DEPENDS moab cgm lasso
    BUILD_IN_SOURCE 1
    ${suppress_build_out}
    BUILD_COMMAND "make -j5 install"
    INSTALL_COMMAND ""
    #PATCH_COMMAND ${GIT_EXECUTABLE} apply ${SuperBuild_PROJECTS_DIR}/patches/meshkit.fix_make_watertight.txt
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
else()
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
      --enable-src
      --enable-utils
      --enable-rgg
      --enable-shared
  )
endif()

if(ENABLE_meshkit)
  find_package(Autotools REQUIRED)
  add_external_project_step(meshkit-autoconf
    COMMAND ${AUTORECONF_EXECUTABLE} -i <SOURCE_DIR> &> <SOURCE_DIR>/AUTOTOOLS_MESSAGES.txt
    DEPENDEES update
    DEPENDERS configure
  )
if(APPLE OR UNIX)
  option(CMBNuc_BUILD_MESHKIT "Build Meshkit." OFF)
  if(CMBNuc_BUILD_MESHKIT)
    include(ExternalProject)
    option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )

    if(BUILD_WITH_CUBIT)
      find_package(CUBIT REQUIRED)
    endif()

    if(APPLE)
      if(BUILD_WITH_CUBIT)
        message("Apple cubit currently only 32 bit, forcing to 32bit")
        set(APPLE_OPTIONS -DCMAKE_OSX_ARCHITECTURES:STRING=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_OSX_SYSROOT:STRING=${CMAKE_OSX_SYSROOT})
      else()
        set(APPLE_OPTIONS -DCMAKE_OSX_ARCHITECTURES:STRING=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_OSX_SYSROOT:STRING=${CMAKE_OSX_SYSROOT})
      endif()
    endif()

    ExternalProject_Add(meshkit SOURCE_DIR ${CMAKE_SOURCE_DIR}/meshkit
                                BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/meshkit
                                CMAKE_ARGS ${APPLE_OPTIONS}
                                           -DCMAKE_BUILD_TYPE:STRING=Release
                                           -DBUILD_WITH_CUBIT:BOOL=${BUILD_WITH_CUBIT}
                                           -DCUBIT_DIR:PATH=${CUBIT_DIR}
                                           -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})


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
