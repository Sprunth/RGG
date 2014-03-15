if(64bit_build)
  set(am 64)
else()
  set(am 32)
endif()

set(boost_with_args --with-date_time --with-filesystem --with-system --with-thread)

#since we don't specify a prefix for the superbuild,
#we can determine where the buld directory will be. This
#is needed as we need to wrap the build directory in quotes to
#and escape spaces in the path for boost to properly build.
string(REPLACE " " "\\ " boost_build_dir ${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost)

#we don't convert the install path on windows, as for the install step it can't
#handle escaping the spaces. I have no clue why boost is
#such a mess on support for directories with spaces in the name

if(MSVC)
  if(MSVC11)
    set(msvc_version "msvc-11.0")
  elseif(MSVC10)
    set(msvc_version "msvc-10.0")
  elseif(MSVC90)
    set(msvc_version "msvc-9.0")
  elseif(MSVC80)
    set(msvc_version "msvc-8.0")
  endif()

  add_external_project(boost
    DEPENDS zlib
    CONFIGURE_COMMAND bootstrap.bat
    BUILD_COMMAND b2 --build-dir=${boost_build_dir} toolset=${msvc_version} address-model=${am} ${boost_with_args}
    INSTALL_COMMAND b2 toolset=${msvc_version} address-model=${am} ${boost_with_args} --prefix=${install_location} install
    BUILD_IN_SOURCE 1
    )
else()
  #ninja / mingw has been launched from a shell with the toolset properly specified
  add_external_project(boost
    DEPENDS zlib
    CONFIGURE_COMMAND bootstrap.bat
    BUILD_COMMAND b2 --build-dir=${boost_build_dir} address-model=${am} ${boost_with_args}
    INSTALL_COMMAND b2 address-model=${am} ${boost_with_args} --prefix=${install_location} install
    BUILD_IN_SOURCE 1
    )
endif()

#install header files
add_external_project_step(fixBoostInstallStage1
    COMMAND ${CMAKE_COMMAND} -E copy_directory <INSTALL_DIR>/include/boost-1_50/boost <INSTALL_DIR>/include/boost
    DEPENDEES install
    )
add_external_project_step(fixBoostInstallStage2
    COMMAND ${CMAKE_COMMAND} -E remove_directory <INSTALL_DIR>/include/boost-1_50
    DEPENDEES fixBoostInstallStage1
    )


add_extra_cmake_args(
  -DBOOST_LIBRARYDIR:PATH=${install_location}/lib
  -DBoost_NO_SYSTEM_PATHS:BOOL=True
)
