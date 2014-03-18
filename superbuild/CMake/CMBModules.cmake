
include (ParaViewModules)

#------------------------------------------------------------------------------
#Macro to hide all non optional CMB modules
macro (SetupCMBSuperBuild)

  #force the variables to be advanced so user don't play with them
  #we want to show the user python as a option
  mark_as_advanced(FORCE
    ENABLE_freetype
    ENABLE_png
    ENABLE_boost
    ENABLE_nuclearRGG
    ENABLE_vtk
    ENABLE_remus
    ENABLE_zeroMQ
    ENABLE_zlib
    )

  set(ENABLE_nuclearRGG ON CACHE BOOL "")

endmacro()


#------------------------------------------------------------------------------
# similar to add_external_project, except provides the user with an option to
# specify to that we just build the dependencies and don't build the primary
# program
macro(add_external_project_or_just_build_dependencies _name)
  if (build-projects)
    add_external_project(${_name} ${ARGN})
  else()
    add_external_project(${_name} ${ARGN})

    set(default_mode "SuperBuild")
    if("${_name}" STREQUAL "cmb")
      set(default_mode "Developer")
    endif()

    set(ENABLE_${_name}_BUILD_MODE "${default_mode}" CACHE STRING
    "Super Build Type. Developer builds minimial cmb dependencies by default. Release builds all dependencies.")
    set_property(CACHE ENABLE_${_name}_BUILD_MODE PROPERTY STRINGS Developer SuperBuild)
  endif()
endmacro()


#------------------------------------------------------------------------------
macro(cmb_process_dependencies)
  set (CM_PROJECTS_ENABLED "")
  foreach(cm-project IN LISTS CM_PROJECTS_ALL)
    set(${cm-project}_ENABLED FALSE)
    if (ENABLE_${cm-project})
      list(APPEND CM_PROJECTS_ENABLED ${cm-project})
    endif()
  endforeach()
  list(SORT CM_PROJECTS_ENABLED) # Deterministic order.

  # Order list to satisfy dependencies. We don't include the use-system
  # libraries in the depedency walk.
  include(TopologicalSort)
  topological_sort(CM_PROJECTS_ENABLED "" _DEPENDS)

  # Now generate a project order using both, optional and non-optional
  # dependencies.
  set (CM_PROJECTS_ORDER ${CM_PROJECTS_ENABLED})
  topological_sort(CM_PROJECTS_ORDER "" _DEPENDS_ANY)

  # Update CM_PROJECTS_ENABLED to be in the correct order taking into
  # consideration optional dependencies.
  set (new_order)
  foreach (cm-project IN LISTS CM_PROJECTS_ORDER)
    list(FIND CM_PROJECTS_ENABLED "${cm-project}" found)
    if (found GREATER -1)
      list(APPEND new_order "${cm-project}")
    endif()
  endforeach()
  set (CM_PROJECTS_ENABLED ${new_order})

  # build information about what project needs what.
  foreach (cm-project IN LISTS CM_PROJECTS_ENABLED)
    enable_project(${cm-project} "")
    foreach (dependency IN LISTS ${cm-project}_DEPENDS)
      enable_project(${dependency} "${cm-project}")
    endforeach()
  endforeach()

  foreach (cm-project IN LISTS CM_PROJECTS_ENABLED)
    if (ENABLE_${cm-project})
      message(STATUS "Enabling ${cm-project} as requested.")
      set_property(CACHE ENABLE_${cm-project} PROPERTY TYPE BOOL)
    else()
      list(SORT ${cm-project}_NEEDED_BY)
      list(REMOVE_DUPLICATES ${cm-project}_NEEDED_BY)
      message(STATUS "Enabling ${cm-project} since needed by: ${${cm-project}_NEEDED_BY}")
      set_property(CACHE ENABLE_${cm-project} PROPERTY TYPE INTERNAL)
    endif()
  endforeach()
  message(STATUS "PROJECTS_ENABLED ${CM_PROJECTS_ENABLED}")
  set (build-projects 1)
  foreach (cm-project IN LISTS CM_PROJECTS_ENABLED)

    #if we are in developer mode for this project it is the same
    #as being a dummy project
    if(ENABLE_${cm-project}_BUILD_MODE STREQUAL "Developer")
      set(${cm-project}_DEVELOPER_PROJECT TRUE)
    else()
      #set it to false so that everything works when a user toggles
      #the setting
      set(${cm-project}_DEVELOPER_PROJECT FALSE)
    endif()

    if (${cm-project}_CAN_USE_SYSTEM)
      # for every enabled project that can use system, expose the option to the
      # user.
      set_property(CACHE USE_SYSTEM_${cm-project} PROPERTY TYPE BOOL)
      if (USE_SYSTEM_${cm-project})
        message(STATUS "usig system ${cm-project}")
        add_external_dummy_project_internal(${cm-project})
        include(${cm-project}.use.system OPTIONAL RESULT_VARIABLE rv)
        if (rv STREQUAL "NOTFOUND")
          message(AUTHOR_WARNING "${cm-project}.use.system not found!!!")
        endif()
      else()
        include(${cm-project})
        add_external_project_internal(${cm-project} "${${cm-project}_ARGUMENTS}")
      endif()
    elseif(${cm-project}_IS_DUMMY_PROJECT)
      #this project isn't built, just used as a graph node to
      #represent a group of dependencies
      add_external_dummy_project_internal(${cm-project})
    elseif(${cm-project}_DEVELOPER_PROJECT)
      #this project isn't built, just used as a graph node to
      #represent a group of dependencies. We write out the search
      #path and cmake args we generally pass to the build to a file
      #so a dev can use that info to build the project outside the superbuild

      include(${cm-project}) #include the project to get the cmake_args
      add_external_developer_project_internal(${cm-project} "${${cm-project}_ARGUMENTS}")
    else()
      include(${cm-project})
      add_external_project_internal(${cm-project} "${${cm-project}_ARGUMENTS}")
    endif()
  endforeach()
  unset (build-projects)
endmacro()

#------------------------------------------------------------------------------
function(add_external_developer_project_internal name)
  #start the parameters to write out to the file with
  #the prefix path that we build everything with. Most well formed
  #FindPackages will only need this set to work.
  set(cmake_cache_args "-DCMAKE_PREFIX_PATH:PATH=${prefix_path}")
  if (CMAKE_BUILD_TYPE)
    list (APPEND cmake_cache_args -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
  endif()

  if (APPLE)
    list (APPEND cmake_cache_args
      -DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
      -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT})
  endif()

  #get extra-cmake args from every dependent project, if any.
  set(arg_DEPENDS)
  get_project_depends(${name} arg)
  foreach(dependency IN LISTS arg_DEPENDS)
    get_property(dependency_args GLOBAL PROPERTY ${dependency}_CMAKE_ARGS)
    list(APPEND cmake_params ${dependency_args})
  endforeach()


  #parse ARGN for all cmake args
  set(insert FALSE)
  set(insert_following_args FALSE)
  foreach(arg IN LISTS ARGN)
    if ("${arg}" MATCHES "CMAKE_ARGS")
      #set insert_following_args true which on the next iteration
      #will set insert to true which makes sure we don't insert CMAKE_ARGS
      #into the list of arguments.
      set(insert_following_args TRUE)
    elseif("${arg}" MATCHES "_ep_keywords_PVExternalProject_Add")
      set(insert FALSE)
    elseif(insert_following_args)
      set(insert TRUE)
      set(insert_following_args FALSE)
    endif()

    if(insert)
      list(APPEND cmake_cache_args "${arg}")
    endif()
  endforeach()

  #add a dummy project that is needed by the _ep_write_initial_cache
  add_external_dummy_project_internal_with_depends(${name} "${arg_DEPENDS}")

  #write out all the cmake vars to a file that can be used to prime a build
  set(cache_file "${SuperBuild_BINARY_DIR}/${name}-Developer-Config.cmake")
  _ep_write_initial_cache(${name} "${cache_file}" "${cmake_cache_args}")

endfunction()
