# - Try to find Remus headers and libraries
#
# Usage of this module as follows:
#
#     find_package(Remus)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Remus_ROOT_DIR  Set this variable to the root installation of
#                            Remus if the module has problems finding
#                            the proper installation path.
#
# Variables defined by this module:
#
#  Remus_FOUND              System has Remus libs/headers
#  Remus_INCLUDE_DIR        The location of Remus headers

if(EXISTS ${MOAB_ROOT_DIR}/lib/MOABConfig.cmake)
   include(${MOAB_ROOT_DIR}/lib/MOABConfig.cmake)
else()

find_path(Remus_ROOT_DIR
    NAMES include/remus/version.h
)

find_path(Remus_INCLUDE_DIR
    NAMES remus/version.h
    HINTS ${Remus_ROOT_DIR}/include/
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Remus DEFAULT_MSG
    Remus_INCLUDE_DIR
)

#now we create fake targets to be used
include(${Remus_ROOT_DIR}/lib/Remus-targets.cmake)

mark_as_advanced(
    Remus_ROOT_DIR
    Remus_INCLUDE_DIR
)
endif()
