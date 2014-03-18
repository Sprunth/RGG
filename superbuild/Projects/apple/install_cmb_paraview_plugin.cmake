
#We only have to do this special installation of the CMB libraries as a plugin
#for ParaView on MAC

set (SHARED_LIBRARY_SUFFIX ".dylib")
execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${TMP_DIR}/CmbPlugin)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${TMP_DIR}/CmbPlugin)

set(plugin_install_dir ${INSTALL_DIR}/Applications/paraview.app/Contents/Plugins)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${plugin_install_dir})

#update the library so that it works properly,
file(GLOB cmbLibs "${CMB_BINARY_DIR}/lib/*Plugin*${SHARED_LIBRARY_SUFFIX}")
foreach(lib ${cmbLibs})
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${lib} ${TMP_DIR}/CmbPlugin)
endforeach()

execute_process(
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_cmb_plugin.py
          # The directory containing the plugin dylibs.
          ${TMP_DIR}/CmbPlugin
          # fixup only the id of the plugins based on this string
          "@executable_path/../Plugins/"
          )
#okay the plugin is fixed up, now we need to install it into paraviews bundle
file(GLOB fixedUpLibs "${TMP_DIR}/CmbPlugin/*${SHARED_LIBRARY_SUFFIX}")
foreach(lib ${fixedUpLibs})
  message(STATUS "instaling plugin: ${lib}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${lib} ${plugin_install_dir})
endforeach()
