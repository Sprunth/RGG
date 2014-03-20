#this will only modify the cppflags for netcdf as the next project in the tree
#moab will unset the cppflags back to the original values
if (build-projects)
  set (pre_netcdf_cpp_flags ${cppflags})
  set (cppflags "-I${install_location}/include ${cppflags}")
endif()

add_external_project(netcdf
  DEPENDS hdf5

  PATCH_COMMAND
    # this patch fixes following issues:
    # 1. incorrect configure target
    # 2. properly use the provided szip find package.
    ${CMAKE_COMMAND} -E copy_if_different ${SuperBuild_PROJECTS_DIR}/patches/netcdf.src.CMakeLists.txt
                                          <SOURCE_DIR>/CMakeLists.txt

  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_UTILITIES:BOOL=ON
    -DENABLE_NETCDF:BOOL=ON
    -DUSE_SZIP:BOOL=ON
    -DUSE_HDF5:BOOL=ON
    -DENABLE_NETCDF_4:BOOL=ON
)

add_external_project_step(patch1
      COMMENT "Fixing missing include files."
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/Projects/patches/netcdf.load.c <SOURCE_DIR>/ncgen3/load.c
      DEPENDEES update
      DEPENDERS patch)
