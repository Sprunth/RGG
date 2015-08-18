include(meshkit.common)

set(BACKUP_CFLAGS ${cflags})
set(BACKUP_CXXFLAGS ${cxxflags})
set(BACKUP_CMAKE_OSX_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
set(cflags "" FORCE)
set(cxxflags "" FORCE)
set(CMAKE_OSX_ARCHITECTURES i386)

add_external_project(meshkit32bit
  APPLE_32Bit
  ${meshkit_args}
    -DCMAKE_OSX_ARCHITECTURES:STRING=i386
    -DBUILD_WITH_CUBIT:BOOL=${BUILD_WITH_CUBIT}
)

set(cflags ${BACKUP_CFLAGS})
set(cxxflags ${BACKUP_CXXFLAGS})
set(CMAKE_OSX_ARCHITECTURES ${BACKUP_CMAKE_OSX_ARCHITECTURES})
