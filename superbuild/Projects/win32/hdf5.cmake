
add_external_project(
  hdf5
  DEPENDS zlib szip

  # HDF5 1.8.9 has a CMake install rule bug. Fix that.
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SuperBuild_PROJECTS_DIR}/patches/hdf5.CMakeLists.txt
                <SOURCE_DIR>/CMakeLists.txt

  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=FALSE
    -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=TRUE
    -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=TRUE
    -DHDF5_ENABLE_SZIP_ENCODING:BOOL=TRUE
    -DHDF5_BUILD_HL_LIB:BOOL=TRUE
)

if (MSVC)
 # hdf5 has a bug with MSVC compiler where it doesn't realize its using MSVC
 # compiler when using nmake or ninja generators. This patch fixes that.
 add_external_project_step(patch_fix_msvc
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${SuperBuild_PROJECTS_DIR}/patches/hdf5.config.cmake.ConfigureChecks.cmake
          <SOURCE_DIR>/config/cmake/ConfigureChecks.cmake
  DEPENDEES update # do after update
  DEPENDERS patch  # do before patch
  )
endif()

# On 32-bit Windows, H5public.h ends up redefining ssize_t. This patch ensures
# that the old definition is undef-ed before redefining it.
if (NOT 64bit_build)
  add_external_project_step(patch_fix_h5public
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SuperBuild_PROJECTS_DIR}/patches/hdf5.src.H5public.h
            <SOURCE_DIR>/src/H5public.h
    DEPENDEES update # do after update
    DEPENDERS patch  # do before patch
  )
endif()

add_extra_cmake_args(
  -DHDF5_DIR:PATH=<INSTALL_DIR>
  -DHDF5_LIBRARIES:FILEPATH=<INSTALL_DIR>/lib/libhdf5_hl${CMAKE_STATIC_LIBRARY_SUFFIX};<INSTALL_DIR/lib/libhdf5${CMAKE_STATIC_LIBRARY_SUFFIX}
  -DHDF5_INCLUDE_DIRS:PATH=<INSTALL_DIR>/include)
