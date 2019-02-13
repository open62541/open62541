find_package(ClangTools)
add_custom_target(clang-tidy ${CLANG_TIDY_PROGRAM}
                  ${lib_sources}
                  -p=compile_commands.json
                  --
                  -I${PROJECT_SOURCE_DIR}/include
                  -I${PROJECT_SOURCE_DIR}/plugins
                  -I${PROJECT_SOURCE_DIR}/deps
                  -I${PROJECT_SOURCE_DIR}/src
                  -I${PROJECT_SOURCE_DIR}/src/server
                  -I${PROJECT_SOURCE_DIR}/src/client
                  -I${PROJECT_BINARY_DIR}/src_generated
                  DEPENDS ${lib_sources}
                  COMMENT "Run clang-tidy on the library")
add_dependencies(clang-tidy open62541)

add_custom_target(cpplint cpplint
                  ${lib_sources}
                  ${internal_headers}
                  ${default_plugin_headers}
                  ${default_plugin_sources}
                  ${ua_architecture_headers}
                  ${ua_architecture_sources}
                  DEPENDS ${lib_sources}
                          ${internal_headers}
                          ${default_plugin_headers}
                          ${default_plugin_sources}
                          ${ua_architecture_headers}
                          ${ua_architecture_sources}
                  COMMENT "Run cpplint code style checker on the library")
