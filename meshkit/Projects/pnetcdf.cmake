find_package(MPI REQUIRED)
get_filename_component(mpi_root "${MPI_C_COMPILER}" DIRECTORY)
get_filename_component(mpi_root "${mpi_root}" DIRECTORY)

add_external_project(pnetcdf
  DEPENDS netcdf

  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
      --disable-fortran
     "--with-mpi=${mpi_root}"
)

add_extra_cmake_args(
  "-DPNetCDF_DIR=<INSTALL_DIR>"
  "-DPNetCDF_INCLUDES=<INSTALL_DIR>/include"
  "-DPNetCDF_LIBRARIES=<INSTALL_DIR>/lib/libpnetcdf.a")
