
if(64bit_build)
  set(am 64)
else()
  set(am 32)
endif()

option(SUPPRESS_BOOST_BUILD_OUTPUT
       "Suppress boost build output"
      ON)
mark_as_advanced(SUPPRESS_BOOST_BUILD_OUTPUT)

set(suppress_build_out)

if(SUPPRESS_BOOST_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

set(boost_with_args --with-date_time --with-filesystem --with-system --with-thread)

#since we don't specify a prefix for the superbuild,
#we can determine where the buld directory will be. This
#is needed as we need to wrap the build directory in quotes to
#and escape spaces in the path for boost to properly build.
string(REPLACE " " "\\ " boost_build_dir ${SuperBuild_BINARY_DIR}/boost/src/boost)
string(REPLACE " " "\\ " boost_install_dir ${install_location})

add_external_project(boost
  DEPENDS zlib
  CONFIGURE_COMMAND ./bootstrap.sh --prefix=${boost_install_dir}
  BUILD_COMMAND ./b2 --build-dir=${boost_build_dir} address-model=${am} ${boost_with_args}
  INSTALL_COMMAND ./b2 address-model=${am} ${boost_with_args} install
  BUILD_IN_SOURCE 1
  ${suppress_build_out}
  )

add_extra_cmake_args(
  -DBoost_NO_SYSTEM_PATHS:BOOL=True
)
