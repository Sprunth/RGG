
#We only have to do this special installation of libraries that don't have
#absolute paths, so that they do have absolute paths, so that people can
#use the libraries from a developer mode version of cmb or smtk
execute_process(
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/fixup_library_rpath.py
          # The directory containing the boost dylibs.
          ${install_location}/lib
          ${lib_name}
          )
