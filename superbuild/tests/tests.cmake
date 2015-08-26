if (RGG_BUILD_PACKAGE)
  add_test(NAME package
           COMMAND "${CMAKE_COMMAND}"
                   "--build" "${CMAKE_BINARY_DIR}"
                   "--target" "package"
                   "--config" "$<CONFIGURATION>")
endif ()
