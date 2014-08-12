

#this will only modify the cppflags for netcdf as the next project in the tree
#moab will unset the cppflags back to the original values
if (build-projects)
  set (pre_netcdf_cpp_flags ${cppflags})
  set (pre_netcdf_c_flags ${cflags})

  set (cppflags "-I${install_location}/include ${cppflags}")
  set (cflags "-I${install_location}/include ${cflags}")
endif()

add_external_project(netcdf
  DEPENDS hdf5

  PATCH_COMMAND
    # this patch fixes following issues:
    # 1. incorrect configure target
    # 2. properly use the provided szip find package.
    ${CMAKE_COMMAND} -E copy_if_different  ${CMAKE_SOURCE_DIR}/Projects/patches/netcdf.CMakeLists.txt
                                          <SOURCE_DIR>/CMakeLists.txt

  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=ON
    -DBUILD_UTILITIES:BOOL=ON
    -DENABLE_NETCDF:BOOL=ON
    -DUSE_SZIP:BOOL=ON
    -DUSE_HDF5:BOOL=ON
    -DENABLE_NETCDF_4:BOOL=ON
    -DSZIP_INCLUDE_DIR:FILEPATH=${install_location}/include
)

ExternalProject_Add_Step(netcdf patch_fix_ncgen3
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
           ${CMAKE_SOURCE_DIR}/Projects/patches/netcdf.ncgen3.load.c
          <SOURCE_DIR>/ncgen3/load.c
  DEPENDEES update # do after update
  DEPENDERS patch  # do before patch
  )


ExternalProject_Get_Property(${name} install_dir)
set_libraries(netcdf ${install_dir}/lib/libnetcdf${CMAKE_SHARED_LIBRARY_SUFFIX})

if(APPLE)
#special mac only script to fixup boost plugin install-names so that developers
#can use the superbuild properly from other projects
ExternalProject_Add_Step(netcdf install_name_fixup
  COMMENT "Fixing netcdf library install_names."
  COMMAND  ${CMAKE_COMMAND}
    -Dinstall_location:PATH=${install_location}
    -Dlib_name:STRING="netcdf"
    -P ${CMAKE_CURRENT_LIST_DIR}/apple/fixup_library_rpath.cmake
  DEPENDEES install)
endif()
