if (CUBIT_FOUND)
  return ()
endif ()

set(CUBITROOT "" CACHE PATH "Where CUBIT is stored")

find_library(CUBIT_LIBRARY
  NAMES cubiti19
  PATHS "${CUBITROOT}/bin"
        "${CUBITROOT}/lib"
        "${CUBITROOT}/libs"
        "${CUBITROOT}/Cubit.app/Contents/MacOS/"
        "${CUBITROOT}/Contents/MacOS/"
        "${CUBITROOT}"
  DOC   "The path to the cubit library")

get_filename_component(CUBIT_PATH "${CUBIT_LIBRARY}" DIRECTORY)

find_library(CUBIT_UTIL_LIBRARY
  NAMES cubit_util
  HINTS "${CUBIT_PATH}"
  PATHS "${CUBITROOT}/bin"
        "${CUBITROOT}/lib"
        "${CUBITROOT}/libs"
        "${CUBITROOT}/Cubit.app/Contents/MacOS/"
        "${CUBITROOT}/Contents/MacOS/"
        "${CUBITROOT}"
  DOC   "The path to the cubit utility library")

find_library(CUBIT_GEOM_LIBRARY
  NAMES cubit_geom
  HINTS "${CUBIT_PATH}"
  PATHS "${CUBITROOT}/bin"
        "${CUBITROOT}/lib"
        "${CUBITROOT}/libs"
        "${CUBITROOT}/Cubit.app/Contents/MacOS/"
        "${CUBITROOT}/Contents/MacOS/"
        "${CUBITROOT}"
  DOC   "The path to the cubit geometry library")

if (CUBIT_LIBRARY)
  add_library(CUBIT::Util SHARED IMPORTED)
  set_property(TARGET CUBIT::Util
    PROPERTY
      IMPORTED_LOCATION "${CUBIT_UTIL_LIBRARY}")
  add_library(CUBIT::Geom SHARED IMPORTED)
  set_property(TARGET CUBIT::Geom
    PROPERTY
      IMPORTED_LOCATION "${CUBIT_GEOM_LIBRARY}")
  add_library(CUBIT::CUBIT SHARED IMPORTED)
  set_property(TARGET CUBIT::CUBIT
    PROPERTY
      IMPORTED_LOCATION "${CUBIT_LIBRARY}")
  set_property(TARGET CUBIT::CUBIT
    PROPERTY
      INTERFACE_LINK_LIBRARIES "CUBIT::Util;CUBIT::Geom")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUBIT
  DEFAULT_MSG
  CUBIT_LIBRARY
  CUBIT_UTIL_LIBRARY
  CUBIT_GEOM_LIBRARY)
