include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/server)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR}/src_generated)

remove_definitions(-std=c99 -Wmissing-prototypes -Wstrict-prototypes)


if(NOT "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
    MESSAGE(FATAL_ERROR "To build fuzzing, you need to use Clang as the compiler")
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    MESSAGE(WARNING "You should use at least clang version 5.0 for fuzzing support")
endif()

if(NOT $ENV{LIB_FUZZER_DIR} STREQUAL "")
    # specific settings for travis
    link_directories($ENV{LIB_FUZZER_DIR})
endif()

set(LIBS Fuzzer ${open62541_LIBRARIES})

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/fuzz)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize-coverage=trace-pc-guard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-coverage=trace-pc-guard")


# Use different plugins for testing
set(fuzzing_plugin_sources ${PROJECT_SOURCE_DIR}/plugins/ua_network_tcp.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_clock.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_config_standard.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c)

add_library(open62541-fuzzplugins OBJECT ${fuzzing_plugin_sources})
add_dependencies(open62541-fuzzplugins open62541)


# the fuzzer test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

add_executable(fuzz_binary_message fuzz_binary_message.cc $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-fuzzplugins>)
target_include_directories(fuzz_binary_message PRIVATE ${PROJECT_SOURCE_DIR}/src/server)
target_link_libraries(fuzz_binary_message ${LIBS})

add_custom_target(
        run_fuzzer
        COMMAND ${CMAKE_BINARY_DIR}/bin/fuzz/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_Browse.bin
        COMMAND ${CMAKE_BINARY_DIR}/bin/fuzz/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_CLO.bin
        COMMAND ${CMAKE_BINARY_DIR}/bin/fuzz/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_CreateActivateSession.bin
        COMMAND ${CMAKE_BINARY_DIR}/bin/fuzz/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_HELOPN.bin
        COMMAND ${CMAKE_BINARY_DIR}/bin/fuzz/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_Read.bin
        COMMAND ${CMAKE_BINARY_DIR}/bin/fuzz/fuzz_binary_message ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_binary_message_corpus/client_Write.bin
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS fuzz_binary_message
        ${MAYBE_USES_TERMINAL})
