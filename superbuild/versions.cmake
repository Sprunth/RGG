# This maintains the links for all sources used by this superbuild.
# Simply update this file to change the revision.
# One can use different revision on different platforms.
# e.g.
# if (UNIX)
#   ..
# else (APPLE)
#   ..
# endif()

get_filename_component(superbuild_top_dir "${CMAKE_SOURCE_DIR}" REALPATH)

add_revision(zlib
  URL "http://www.paraview.org/files/dependencies/zlib-1.2.7.tar.gz"
  URL_MD5 60df6a37c56e7c1366cca812414f7b85)
# NOTE: if updating zlib version, fix patch in zlib.cmake

add_revision(png
  URL "http://paraview.org/files/dependencies/libpng-1.4.8.tar.gz"
  URL_MD5 49c6e05be5fa88ed815945d7ca7d4aa9)

add_revision(freetype
  URL "http://paraview.org/files/dependencies/freetype-2.4.8.tar.gz"
  URL_MD5 "5d82aaa9a4abc0ebbd592783208d9c76")

add_revision(szip
  URL "http://paraview.org/files/dependencies/szip-2.1.tar.gz"
  URL_MD5 902f831bcefb69c6b635374424acbead)

add_revision(hdf5
  URL "http://paraview.org/files/dependencies/hdf5-1.8.9.tar.gz"
  URL_MD5 d1266bb7416ef089400a15cc7c963218)

add_revision(libxml2
  URL "http://paraview.org/files/dependencies/libxml2-2.7.8.tar.gz"
  URL_MD5 8127a65e8c3b08856093099b52599c86)

add_revision(fontconfig
  URL "http://paraview.org/files/dependencies/fontconfig-2.8.0.tar.gz"
  URL_MD5 77e15a92006ddc2adbb06f840d591c0e)

add_revision(qt
  URL "http://download.qt-project.org/official_releases/qt/4.8/4.8.6/qt-everywhere-opensource-src-4.8.6.tar.gz"
  URL_MD5 2edbe4d6c2eff33ef91732602f3518eb)

add_revision(boost
  URL "http://downloads.sourceforge.net/project/boost/boost/1.56.0/boost_1_56_0.tar.gz"
  URL_MD5 8c54705c424513fa2be0042696a3a162 )

add_revision( nuclearRGG SOURCE_DIR ${superbuild_top_dir}/.. )

add_revision(vtk
  GIT_REPOSITORY http://vtk.org/VTK.git
  GIT_TAG eb5f6f38fe05c8c5fc3b6d488efdb41bd0edf50f
)

add_revision(zeroMQ
  GIT_REPOSITORY "https://github.com/robertmaynard/zeromq4-x.git"
  GIT_TAG "master"
)

add_revision(remus
  GIT_REPOSITORY https://github.com/Kitware/Remus.git
  GIT_TAG 8cd2d545b09b2282213c7afd581c74731ec775a5
)

add_revision(QtTesting
  GIT_REPOSITORY "http://paraview.org/QtTesting.git"
  GIT_TAG master)

add_revision(meshkit SOURCE_DIR ${superbuild_top_dir}/../meshkit/ )
add_revision(meshkit32bit SOURCE_DIR ${superbuild_top_dir}/../meshkit/ )

#------------------------------------------------------------------------------
# moab versions
#------------------------------------------------------------------------------
add_revision(netcdf
  URL "ftp://ftp.unidata.ucar.edu/pub/netcdf/old/netcdf-4.3.0.tar.gz"
  URL_MD5 40c0e53433fc5dc59296ee257ff4a813)

add_revision(netcdfcpp
  URL "ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-cxx-4.2.tar.gz"
  URL_MD5 d32b20c00f144ae6565d9e98d9f6204c)

add_revision(pnetcdf
  URL "http://cucis.ece.northwestern.edu/projects/PnetCDF/Release/parallel-netcdf-1.6.0.tar.bz2"
  URL_MD5 43e1ce63da7aab72829502a1e2e27161)

# FIXME: verdict support?
add_revision(moab
             GIT_REPOSITORY https://bitbucket.org/mathstuf/moab.git
             GIT_TAG windows-with-cmake-support)
