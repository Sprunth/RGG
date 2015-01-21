option(SUPPRESS_meshkit_BUILD_OUTPUT
"Suppress meshkit build output"
ON)
mark_as_advanced(SUPPRESS_meshkit_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_meshkit_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

if(BUILD_WITH_CUBIT)

find_package(Autotools REQUIRED)
add_external_project(meshkit
    BUILD_COMMAND "make -j5 install"
    INSTALL_COMMAND ""
    ${suppress_build_out}
    CMAKE_ARGS
      -DBUILD_MESHKIT_MASTER:BOOL=${BUILD_MESHKIT_MASTER}
      -DAUTOHEADER_EXECUTABLE:path=${AUTOHEADER_EXECUTABLE}
      -DAUTOM4TE_EXECUTABLE:path=${AUTOM4TE_EXECUTABLE}
      -DAUTORECONF_EXECUTABLE:path=${AUTORECONF_EXECUTABLE}
      -DAUTOUPDATE_EXECUTABLE:path=${AUTOUPDATE_EXECUTABLE}
      -DBUILD_WITH_CUBIT:BOOL=ON
      -DCMAKE_INSTALL_PREFIX:path=<INSTALL_DIR>/meshkitCubit
      -DCMAKE_PREFIX_PATH:path=<INSTALL_DIR>/meshkitCubit
      -DAUTOHEADER_EXECUTABLE:path=${AUTOHEADER_EXECUTABLE}
      -DAUTOM4TE_EXECUTABLE:path=${AUTOM4TE_EXECUTABLE}
      -DAUTORECONF_EXECUTABLE:path=${AUTORECONF_EXECUTABLE}
      -DAUTOUPDATE_EXECUTABLE:path=${AUTOUPDATE_EXECUTABLE}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DCUBIT_DIR:PATH=${CUBIT_DIR})
else()

find_package(Autotools REQUIRED)
add_external_project(meshkit
  ${suppress_build_out}
  DEPENDS moab cgm lasso
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DENABLE_ALGS:BOOL=ON
    -DENABLE_SRC:BOOL=ON
    -DENABLE_UTILS:BOOL=ON
    -DENABLE_RGG:BOOL=ON
)

endif()

if(ENABLE_meshkit)
  option ( BUILD_WITH_CUBIT       "Build CGM with CUBIT"                 OFF )
endif()
