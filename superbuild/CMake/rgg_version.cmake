# We hardcode the version numbers since we cannot determine versions during
# configure stage.
set (rgg_version_major 4)
set (rgg_version_minor 1)
set (rgg_version_patch 1)
set (rgg_version_suffix)
set (rgg_version "${rgg_version_major}.${rgg_version_minor}")
if (rgg_version_suffix)
  set (rgg_version_long "${rgg_version}.${rgg_version_patch}-${rgg_version_suffix}")
else()
  set (rgg_version_long "${rgg_version}.${rgg_version_patch}")
endif()
