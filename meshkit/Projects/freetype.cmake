add_external_dummy_project(freetype)

add_extra_cmake_args(
  -DFREETYPE_INCLUDE_DIR_freetype2=${FREETYPE_INCLUDE_DIR_freetype2}
  -DFREETYPE_INCLUDE_DIR_ft2build=${FREETYPE_INCLUDE_DIR_ft2build}
  -DFREETYPE_LIBRARY=${FREETYPE_LIBRARY}
)
