option(SUPPRESS_LASSO_BUILD_OUTPUT
"Suppress LASSO build output"
ON)
mark_as_advanced(SUPPRESS_LASSO_BUILD_OUTPUT)

set(suppress_build_out)
if(SUPPRESS_LASSO_BUILD_OUTPUT)
set(suppress_build_out SUPPRESS_BUILD_OUTPUT)
endif()

add_external_project(lasso
  ${suppress_build_out}
  DEPENDS moab cgm
  CMAKE_ARGS
    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
    -DLASSO_ENABLE_FORTRAN:BOOL=FALSE)
