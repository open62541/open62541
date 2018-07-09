include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/server)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_SOURCE_DIR}/tests/testing-plugins)
include_directories(${PROJECT_BINARY_DIR}/src_generated)

if(NOT MSVC)
    add_definitions(-Wno-deprecated-declarations)
endif()

if (UA_BUILD_FUZZING_CORPUS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_definitions(-DUA_CORPUS_OUTPUT_DIR="${PROJECT_BINARY_DIR}/corpus/custom")
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/corpus/custom)

    add_executable(corpus_generator ua_debug_dump_pkgs_file.c corpus_generator.c)
    target_link_libraries(corpus_generator open62541 ${open62541_LIBRARIES})
    assign_source_group(corpus_generator)
    add_dependencies(corpus_generator open62541-amalgamation-header)
    set_target_properties(corpus_generator PROPERTIES FOLDER "open62541/corpus_generator")
    set_target_properties(corpus_generator PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    return()
endif()


remove_definitions(-std=c99 -Wmissing-prototypes -Wstrict-prototypes)
set (CMAKE_CXX_STANDARD 11)

if (NOT UA_BUILD_OSS_FUZZ)

    if(NOT "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
        MESSAGE(FATAL_ERROR "To build fuzzing, you need to use Clang as the compiler")
    endif()

    # oss-fuzz builds already include these flags
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp")
    endif()
    set(LIBS Fuzzer)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(LIBS $ENV{LIB_FUZZING_ENGINE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{OUT})
endif()
list(APPEND LIBS "${open62541_LIBRARIES}")

# Use different plugins for testing
set(fuzzing_plugin_sources ${PROJECT_SOURCE_DIR}/arch/ua_network_tcp.c
        ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_clock.c
        ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_networklayers.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_log_stdout.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_config_default.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_nodestore_default.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_accesscontrol_default.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_pki_certificate.c
        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_none.c
)

if(UA_ENABLE_ENCRYPTION)
    set(fuzzing_plugin_sources ${fuzzing_plugin_sources}
        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic128rsa15.c)
    set(fuzzing_plugin_sources ${fuzzing_plugin_sources}
        ${PROJECT_SOURCE_DIR}/plugins/ua_securitypolicy_basic256sha256.c)
endif()

add_library(open62541-fuzzplugins OBJECT ${fuzzing_plugin_sources} ${PROJECT_SOURCE_DIR}/arch/${UA_ARCHITECTURE}/ua_architecture_functions.c)
add_dependencies(open62541-fuzzplugins open62541)

# the fuzzer test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

set(FUZZER_TARGETS)

macro(add_fuzzer FUZZER_NAME FUZZER_SOURCE)
    add_executable(${FUZZER_NAME} $<TARGET_OBJECTS:open62541-object> $<TARGET_OBJECTS:open62541-fuzzplugins> ${FUZZER_SOURCE} ${ARGN})
    target_link_libraries(${FUZZER_NAME} ${LIBS})
    target_include_directories(${FUZZER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/server)
    assign_source_group(${FUZZER_SOURCE})
    list(APPEND FUZZER_TARGETS ${FUZZER_NAME})
endmacro()

# Add new fuzzers here
add_fuzzer(fuzz_binary_message fuzz_binary_message.cc)
add_fuzzer(fuzz_binary_decode fuzz_binary_decode.cc)

file(GLOB CORPUS_FILES ${PROJECT_SOURCE_DIR}/tests/fuzz/fuzz_binary_message_corpus/generated/*)

SET(CORPUS_CMDS "")
FOREACH(f ${CORPUS_FILES})
    LIST(APPEND CORPUS_CMDS COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message "${f}")
ENDFOREACH(f)

add_custom_target(run_fuzzer ${CORPUS_CMDS}
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS ${FUZZER_TARGETS}
                  ${MAYBE_USES_TERMINAL})
