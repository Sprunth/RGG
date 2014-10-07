#windows meshkit, do not do anything
add_external_dummy_project_internal_with_depends(meshkit)
set(ENABLE_OCE false FORCE)
set(ENABLE_cgm false FORCE)
set(ENABLE_lasso false FORCE)