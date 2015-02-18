# This maintains the links for all sources used by this superbuild.
# Simply update this file to change the revision.
# One can use different revision on different platforms.
# e.g.
# if (UNIX)
#   ..
# else (APPLE)
#   ..
# endif()


add_revision(mpich2
  URL "http://www.mcs.anl.gov/research/projects/mpich2/downloads/tarballs/1.4.1p1/mpich2-1.4.1p1.tar.gz"
  URL_MD5 b470666749bcb4a0449a072a18e2c204)

add_revision(ftgl
  GIT_REPOSITORY "https://github.com/ulrichard/ftgl.git"
  GIT_TAG cf4d9957930e41c3b82a57b20207242c7ef69f18
  )

add_revision(OCE
  GIT_REPOSITORY "https://github.com/mathstuf/oce.git"
  GIT_TAG "next"
  )

add_revision(pnetcdf
  URL "http://cucis.ece.northwestern.edu/projects/PnetCDF/Release/parallel-netcdf-1.6.0.tar.bz2"
  URL_MD5 43e1ce63da7aab72829502a1e2e27161)

add_revision(cgm
  GIT_REPOSITORY "https://bitbucket.org/mathstuf/cgm.git"
  GIT_TAG windows-with-cmake-support
  )

if(BUILD_MESHKIT_MASTER)
  add_revision(moab
    GIT_REPOSITORY https://bitbucket.org/mathstuf/moab.git
    GIT_TAG windows-with-cmake-support
  )

  add_revision(lasso
    GIT_REPOSITORY https://bitbucket.org/mathstuf/lasso.git
    GIT_TAG cmake
  )

  add_revision(meshkit
    GIT_REPOSITORY https://bitbucket.org/mathstuf/meshkit.git
    GIT_TAG cmake
  )
else()
  add_revision( moab
                GIT_REPOSITORY https://bitbucket.org/judajake/moab.git
                GIT_TAG add_verdict_support )

  add_revision(lasso
    GIT_REPOSITORY https://bitbucket.org/fathomteam/lasso.git
    GIT_TAG Version3.2
  )

  add_revision(meshkit
    #GIT_REPOSITORY https://bitbucket.org/fathomteam/meshkit.git
    GIT_REPOSITORY https://bitbucket.org/judajake/meshkit.git
    #GIT_TAG MeshKitv1.3
    GIT_TAG v3
    #GIT_TAG 57659678d1632bcac7a4f551cad7800a40c27aa4
  )
endif()
