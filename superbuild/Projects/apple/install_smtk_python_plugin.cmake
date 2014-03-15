
#We only have to do this special installation of the smtk libraries as a plugin
#on MAC

execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${TMP_DIR}/SMTKPlugin)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${TMP_DIR}/SMTKPlugin)

set(plugin_install_dir ${INSTALL_DIR}/Applications/ModelBuilder.app/Contents/Python/smtk)
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${plugin_install_dir})

set(py_files_libs ${SMTK_BIN_DIR}/lib/SMTKCorePython.so 
                  ${SMTK_BIN_DIR}/lib/libshiboken-python${PYTHON_VERSION}.1.2.dylib
                  ${SMTK_BIN_DIR}/lib/python${PYTHON_VERSION}/site-packages/shiboken.so)
foreach(lib ${py_files_libs})
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${lib} ${TMP_DIR}/SMTKPlugin)
endforeach()

execute_process(
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_smtk_python.py
          # The directory containing the plugin dylibs.
          ${TMP_DIR}/SMTKPlugin
          # fixup only the id of the plugins based on this string
          ""
          )
          
execute_process(
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/../remove_code.py
          ${SMTK_BIN_DIR}/python/smtk.py
          ${TMP_DIR}/SMTKPlugin/__init__.py
          )

#okay the plugin is fixed up, now we need to install it into paraviews bundle
file(GLOB fixedUpLibs "${TMP_DIR}/SMTKPlugin/*")
foreach(lib ${fixedUpLibs})
  message(STATUS "instaling plugin: ${lib}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${lib} ${plugin_install_dir})
endforeach()