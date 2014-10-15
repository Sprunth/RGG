
find_library(MOAB_LIBRARY MOAB REQUIRED)

# If SiMBA is installed, use SiMBA/bld/install/include for MOAB_INCLUDE_DIRS
find_path(MOAB_INCLUDE_DIRS Core.hpp REQUIRED)
include_directories(${MOAB_INCLUDE_DIRS})


find_package(VTK)
include(${VTK_USE_FILE})

set(vtk_library_headers
  SimpleMoab.h
  CellTypeToType.h
  DataSetConverter.h
  MixedCellConnectivity.h
  vtkMoabReader.h
  )

set(vtk_library_src
    vtkMoabReader.cxx )



target_link_libraries(vtkMoabReaderPlugin
                      ${MOAB_LIBRARY})
