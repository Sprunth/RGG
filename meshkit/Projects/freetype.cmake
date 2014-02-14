set(CMAKE_C_FLAGS ${cflags})
message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
add_external_project(
  freetype
  DEPENDS zlib
  CONFIGURE_COMMAND <SOURCE_DIR>/configure
                    --prefix=<INSTALL_DIR>
                    --enable-static=no
                     --with-sysroot=<INSTALL_DIR>
                     "CFLAGS=${cflags}" "CPPFLAGS=${cppflags}"
)
