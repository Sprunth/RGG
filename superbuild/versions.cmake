# This maintains the links for all sources used by this superbuild.
# Simply update this file to change the revision.
# One can use different revision on different platforms.
# e.g.
# if (UNIX)
#   ..
# else (APPLE)
#   ..
# endif()

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

add_revision(silo
  URL "http://paraview.org/files/dependencies/silo-4.8-bsd.tar.gz"
  URL_MD5 d864e383f25b5b047b98aa2d5562d379)

add_revision(cgns
  URL "http://www.paraview.org/files/dependencies/cgnslib_3.1.3-4.tar.gz"
  URL_MD5 442bba32b576f3429cbd086af43fd4ae)

add_revision(ffmpeg
  URL "http://paraview.org/files/dependencies/ffmpeg-0.6.5.tar.gz"
  URL_MD5 451054dae3b3d33a86c2c48bd12d56e7)

add_revision(libxml2
  URL "http://paraview.org/files/dependencies/libxml2-2.7.8.tar.gz"
  URL_MD5 8127a65e8c3b08856093099b52599c86)

add_revision(fontconfig
  URL "http://paraview.org/files/dependencies/fontconfig-2.8.0.tar.gz"
  URL_MD5 77e15a92006ddc2adbb06f840d591c0e)

add_revision(qt
  URL "http://download.qt-project.org/official_releases/qt/4.8/4.8.6/qt-everywhere-opensource-src-4.8.6.tar.gz"
  URL_MD5 2edbe4d6c2eff33ef91732602f3518eb)

if (WIN32 OR (CROSS_BUILD_STAGE STREQUAL "CROSS"))
  add_revision(python
    URL "http://www.paraview.org/files/dependencies/Python-2.7.3.tgz"
    URL_MD5 "2cf641732ac23b18d139be077bd906cd")
else()
  add_revision(python
    URL "http://paraview.org/files/dependencies/Python-2.7.2.tgz"
    URL_MD5 "0ddfe265f1b3d0a8c2459f5bf66894c7")
endif()

add_revision(numpy
  URL "http://paraview.org/files/dependencies/numpy-1.6.2.tar.gz"
  URL_MD5 95ed6c9dcc94af1fc1642ea2a33c1bba)

add_revision(matplotlib
  URL "http://paraview.org/files/dependencies/matplotlib-1.1.1_notests.tar.gz"
  URL_MD5 30ee59119599331bf1f3b6e838fee9a8)

add_revision(boost
  URL "http://www.paraview.org/files/dependencies/boost_1_50_0.tar.gz"
  URL_MD5 dbc07ab0254df3dda6300fd737b3f264)

add_revision(manta
  URL "http://paraview.org/files/dependencies/manta-r2439.tar.gz"
  URL_MD5 fbf4107fe2f6d7e8a5ae3dda71805bdc)

add_revision(molequeue
  GIT_REPOSITORY git://source.openchemistry.org/molequeue.git
  GIT_TAG e545ee25b4b79d5bae9e1)

add_revision( nuclearRGG SOURCE_DIR ${CMAKE_SOURCE_DIR}/.. )

add_revision(vtk
  GIT_REPOSITORY git://vtk.org/VTK.git
  GIT_TAG v6.1.0)

if (UNIX)
  add_revision(mpi
    URL "http://paraview.org/files/dependencies/mpich2-1.4.1p1.tar.gz"
    URL_MD5 b470666749bcb4a0449a072a18e2c204)
elseif (WIN32)
  add_revision(mpi
    URL "http://www.paraview.org/files/dependencies/openmpi-1.4.4.tar.gz"
    URL_MD5 7253c2a43445fbce2bf4f1dfbac113ad)
endif()

if (CROSS_BUILD_STAGE STREQUAL "CROSS")
  add_revision(mesa
    URL "http://www.paraview.org/files/dependencies/MesaLib-7.6.1.tar.gz"
    URL_MD5 e80fabad2e3eb7990adae773d6aeacba)
else()
  add_revision(mesa
    URL "http://paraview.org/files/dependencies/MesaLib-7.11.2.tar.gz"
    URL_MD5 b9e84efee3931c0acbccd1bb5a860554)
endif()

# We stick with 7.11.2 for Mesa version for now. Newer mesa doesn't seem to
# build correctly with certain older compilers (e.g. on neser).
add_revision(osmesa
    URL "http://paraview.org/files/dependencies/MesaLib-7.11.2.tar.gz"
    URL_MD5 b9e84efee3931c0acbccd1bb5a860554)

if(USE_PARAVIEW_next)
  add_revision(paraview
    GIT_REPOSITORY http://paraview.org/ParaView.git
    GIT_TAG next)
elseif(USE_PARAVIEW_master)
  add_revision(paraview
    GIT_REPOSITORY http://paraview.org/ParaView.git
    GIT_TAG master)
else()
  add_revision(paraview
    # updating to a version of ParaView that fixes issue loading a
    # plugin multiple times which destroyed our dashboard
    GIT_REPOSITORY http://paraview.org/ParaView.git
    GIT_TAG e90304c0053a68ab316790748e097d810cb17ed1)
endif()

#------------------------------------------------------------------------------
# Optional Plugins. Doesn't affect ParaView binaries at all even if missing
# or disabled.
#------------------------------------------------------------------------------

add_revision(cosmologytools
    GIT_REPOSITORY http://public.kitware.com/cosmotools.git
    GIT_TAG v0.11)

add_revision(qhull
    GIT_REPOSITORY http://github.com/gzagaris/gxzagas-qhull.git
    GIT_TAG master)


if (TRUST_SVN_CERTIFICATES_AUTOMATICALLY)
  add_revision(diy
     SVN_REPOSITORY https://svn.mcs.anl.gov/repos/diy/trunk
     SVN_REVISION -r176
     SVN_TRUST_CERT 1)
else()
  add_revision(diy
     SVN_REPOSITORY https://svn.mcs.anl.gov/repos/diy/trunk
     SVN_REVISION -r176)
endif()

add_revision(portfwd
  URL "http://www.paraview.org/files/dependencies/portfwd-0.29.tar.gz"
  URL_MD5 93161c91e12b0d67ca52dc13708a2f2f)

add_revision(vxl
  URL http://vtk.org/files/support/vxl_r35313.tar.gz
  URL_MD5 cb7884a8eb0f816a4bb378dc968c8333)

if (WIN32)
  #windows uses the custom cmake build of zeroMQ
  add_revision(zeroMQ
    GIT_REPOSITORY https://github.com/robertmaynard/zeromq2-x
    GIT_TAG master)
else()
  #unix and mac uses the stable release
  add_revision(zeroMQ
    URL "http://download.zeromq.org/zeromq-2.2.0.tar.gz"
    URL_MD5 1b11aae09b19d18276d0717b2ea288f6)

endif()

add_revision(remus
  GIT_REPOSITORY http://public.kitware.com/Remus.git
  GIT_TAG 6b382bd4df55585a96adfae9dd9d6e7198a25b23
  )

add_revision(kml
  URL http://vtk.org/files/support/libkml_fa6c7d8.tar.gz
  URL_MD5 261b39166b18c2691212ce3495be4e9c
  )

add_revision(gdal
  URL http://vtk.org/files/support/gdal_5b8309b.tar.gz
  URL_MD5 8d77df722a01cc86b292300a007a3282
  )

#------------------------------------------------------------------------------
# moab versions
#------------------------------------------------------------------------------
add_revision(ftgl
  GIT_REPOSITORY "https://github.com/ulrichard/ftgl.git"
  GIT_TAG cf4d9957930e41c3b82a57b20207242c7ef69f18
  )

add_revision(oce
  GIT_REPOSITORY "https://github.com/robertmaynard/oce.git"
  GIT_TAG "cgm_support"
  )

add_revision(netcdf
  URL "ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-4.3.0.tar.gz"
  URL_MD5 40c0e53433fc5dc59296ee257ff4a813
  )

add_revision(netcdfcpp
  URL "ftp://ftp.unidata.ucar.edu/pub/netcdf/netcdf-cxx-4.2.tar.gz"
  URL_MD5 d32b20c00f144ae6565d9e98d9f6204c
)

add_revision(cgm
  GIT_REPOSITORY https://bitbucket.org/fathomteam/cgm.git
  GIT_TAG 13.1.1
  )

add_revision(moab
  GIT_REPOSITORY https://judajake@bitbucket.org/judajake/moab.git
  GIT_TAG 4.6.3_cmake)
