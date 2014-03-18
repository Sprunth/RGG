option(SUPPRESS_PNG_BUILD_OUTPUT
       "Suppress PNG build output"
      ON)
mark_as_advanced(SUPPRESS_PNG_BUILD_OUTPUT)

set(suppress_build_out)

if(SUPPRESS_PNG_BUILD_OUTPUT)
  set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(png
  DEPENDS zlib

  CMAKE_ARGS
    -DPNG_TESTS:BOOL=OFF
    # VTK uses API that gets hidden when PNG_NO_STDIO is TRUE (default).
    -DPNG_NO_STDIO:BOOL=OFF
    # Always build in release mode since the symlinks on Linux don't
    # work right for debug mode (libpng.{so,a} points to libpng14.{so,a}
    # instead of libpng14d.{so,a}).
    -DCMAKE_BUILD_TYPE:STRING=Release
  ${suppress_build_out}
  )
