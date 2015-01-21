option(SUPPRESS_CGM_BUILD_OUTPUT
"Suppress CGM build output"
ON)
mark_as_advanced(SUPPRESS_CGM_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_CGM_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

set(cgm_cmake_args)
if (BUILD_WITH_CUBIT)
  list(APPEND cgm_cmake_args
    "-DCUBIT_DIR:PATH=${CUBIT_DIR}")
endif ()

#we need a external build step since cgm expects occ to be in the system
#library search path.
#string(REPLACE " " "\\ " cgm_binary "${CMAKE_CURRENT_BINARY_DIR}/cgm/src/cgm")
#set(BUILD_STEP ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cgm_build_step.cmake)
#configure_file(${patch_location}/cgm_build_step.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cgm_build_step.cmake @ONLY)
add_external_project(cgm
  ${suppress_build_out}
  CMAKE_ARGS
    ${cgm_cmake_args}
    -DCGM_KCM:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS})

set_property(GLOBAL APPEND
  PROPERTY
    cgm_CMAKE_ARGS "-DCGM_DIR=<INSTALL_DIR>/lib/cmake/CGM")
