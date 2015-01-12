set(CUBIT_DIR "" CACHE PATH "Where CUBIT is stored")

find_library(CUBIT_LIBRARY
  NAMES cubiti19
  PATHS "${CUBIT_DIR}/bin"
        "${CUBIT_DIR}/lib"
        "${CUBIT_DIR}/libs"
        "${CUBIT_DIR}"
  DOC   "The path to the cubit library")
if (CUBIT_LIBRARY)
  get_filename_component(CUBIT_PATH "${CUBIT_LIBRARY}" DIRECTORY)
  add_library(CUBIT::CUBIT SHARED IMPORTED)
  set_property(TARGET CUBIT::CUBIT
    PROPERTY
      IMPORTED_LOCATION "${CUBIT_LIBRARY}")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUBIT
  DEFAULT_MSG
  CUBIT_LIBRARY)
