# this file determines binary dependencies for ParaView and install thems.

# dependencies_root == directory where cmb external dependencies are installed.
# pv_dependencies_root == directory where paraview dependencies are installed.
# target_root == root directory where files are to be installed.


function(gp_resolve_item_override context item exepath  dirs resolved_item resolved)
  #it seems in some version of cmake GetPrerequisites doesn't properly search all directories we pass
  #in, so instead we use override hooks to find libraries.
  #we will search where we installed the paraview libraries and in the bin directory
  #of install, since that is where some libs are placed
  find_file(ri "${item}" PATHS ${pv_dependencies_root} NO_DEFAULT_PATH)
  if(ri)
    set(${resolved} 1 PARENT_SCOPE)
    set(${resolved_item} "${ri}" PARENT_SCOPE)
  endif()
endfunction(gp_resolve_item_override)

include(GetPrerequisites)

get_filename_component(exepath "${executable}" PATH)
get_filename_component(exename "${executable}" NAME)

message(STATUS "Determining dependencies for '${exename}'")
get_prerequisites(
  ${executable}
  prerequisites
  1
  1
  ${exepath}
   ${dependencies_root}/lib
   )

message(STATUS "Installing dependencies for '${exename}'")


#we are getting a problem that prerequisites that we solve in gp_resolve_item_override
#aren't storing the full system path, so lets fix-up the prerequisites
set(resolved_prerequisites)
foreach(link ${prerequisites})
  set(full_path full_path-NOTFOUND)
  get_filename_component(linkname "${link}" NAME)
  find_file(full_path "${linkname}" PATHS ${pv_dependencies_root} NO_DEFAULT_PATH)

  if(NOT full_path)
    set(full_path ${link})
  endif()

  if (NOT link MATCHES ".*fontconfig.*")
    if (IS_SYMLINK ${full_path})
      get_filename_component(resolved_link "${full_path}" REALPATH)
      # now link may not directly point to resolved_link.
      # so we install the resolved link as the link.
      get_filename_component(resolved_name "${full_path}" NAME)
      file(INSTALL
        DESTINATION "${target_root}"
        TYPE PROGRAM
        RENAME "${resolved_name}"
      FILES "${resolved_link}")
    else ()
      list(APPEND resolved_prerequisites ${full_path})
    endif()
  endif()
endforeach()

file(INSTALL ${resolved_prerequisites}
     DESTINATION ${target_root}
     USE_SOURCE_PERMISSIONS)
