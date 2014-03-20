option(SUPPRESS_HDF5_BUILD_OUTPUT
       "Suppress HDF5 build output"
      ON)
mark_as_advanced(SUPPRESS_HDF5_BUILD_OUTPUT)

set(suppress_build_out)

if(SUPPRESS_HDF5_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(
  hdf5
  DEPENDS zlib szip

  # HDF5 1.8.9 has a CMake install rule bug. Fix that.
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SuperBuild_PROJECTS_DIR}/patches/hdf5.CMakeLists.txt
                <SOURCE_DIR>/CMakeLists.txt

  #always build in release mode.
  #enable install name so that we get full paths to library on apple
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE:STRING=Release
    -DBUILD_SHARED_LIBS:BOOL=TRUE
    -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=TRUE
    -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=TRUE
    -DHDF5_ENABLE_SZIP_ENCODING:BOOL=TRUE
    -DHDF5_BUILD_HL_LIB:BOOL=TRUE
    -DHDF5_BUILD_WITH_INSTALL_NAME:BOOL=TRUE
  ${suppress_build_out}
)
