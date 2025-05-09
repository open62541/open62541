get_property(open62541_BUILD_INCLUDE_DIRS TARGET open62541 PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${open62541_BUILD_INCLUDE_DIRS})
include_directories("${PROJECT_SOURCE_DIR}/deps")
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src/server")
include_directories("${PROJECT_SOURCE_DIR}/tests/testing-plugins")

if(NOT MSVC)
    add_definitions(-Wno-deprecated-declarations)
endif()

if (UA_BUILD_FUZZING_CORPUS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_definitions(-DUA_CORPUS_OUTPUT_DIR="${PROJECT_BINARY_DIR}/corpus/custom")
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/corpus/custom)

    add_executable(corpus_generator ua_debug_dump_pkgs_file.c corpus_generator.c
                                    ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_networklayers.c)
    target_link_libraries(corpus_generator open62541 ${open62541_LIBRARIES})
    assign_source_group(corpus_generator)
    set_target_properties(corpus_generator PROPERTIES FOLDER "open62541/corpus_generator")
    set_target_properties(corpus_generator PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    return()
endif()

remove_definitions(-std=c99 -Wmissing-prototypes -Wstrict-prototypes)
set(CMAKE_CXX_STANDARD 11)

if(NOT UA_BUILD_OSS_FUZZ)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        MESSAGE(FATAL_ERROR "To build fuzzing, you need to use Clang as the compiler")
    endif()

    # oss-fuzz builds already include these flags
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address")
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=fuzzer-no-link")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=fuzzer-no-link")
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
        set(LIBS Fuzzer)
    endif()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(LIBS $ENV{LIB_FUZZING_ENGINE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{OUT})
endif()

list(APPEND LIBS "${open62541_LIBRARIES}")
link_libraries("-fsanitize=fuzzer")

# the fuzzer test are built directly on the open62541 object files. so they can
# access symbols that are hidden/not exported to the shared library

set(FUZZER_TARGETS)

macro(add_fuzzer FUZZER_NAME FUZZER_SOURCE)
    add_executable(${FUZZER_NAME} ${FUZZER_SOURCE}
                                  ${PROJECT_SOURCE_DIR}/tests/testing-plugins/testing_networklayers.c
                                  $<TARGET_OBJECTS:open62541-object>
                                  $<TARGET_OBJECTS:open62541-plugins>)
    target_link_libraries(${FUZZER_NAME} ${LIBS})
    target_include_directories(${FUZZER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/server)
    target_include_directories(${FUZZER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/deps/mdnsd)
    assign_source_group(${FUZZER_SOURCE})
    list(APPEND FUZZER_TARGETS ${FUZZER_NAME})
endmacro()

# Add new fuzzers here
add_fuzzer(fuzz_binary_message fuzz_binary_message.cc)
add_fuzzer(fuzz_tcp_message fuzz_tcp_message.cc)
add_fuzzer(fuzz_binary_decode fuzz_binary_decode.cc)
add_fuzzer(fuzz_src_ua_util fuzz_src_ua_util.cc)
add_fuzzer(fuzz_attributeoperand fuzz_attributeoperand.cc)

# Add fuzzer for mdns dependency. It's currently not fuzzed separately.
# See also: https://github.com/google/oss-fuzz/pull/2928
add_fuzzer(fuzz_mdns_message ${PROJECT_SOURCE_DIR}/deps/mdnsd/tests/fuzz/fuzz_mdns_message.cc)
add_fuzzer(fuzz_mdns_xht ${PROJECT_SOURCE_DIR}/deps/mdnsd/tests/fuzz/fuzz_mdns_xht.cc)

add_fuzzer(fuzz_base64_encode fuzz_base64_encode.cc)
add_fuzzer(fuzz_base64_decode fuzz_base64_decode.cc)

if(UA_ENABLE_JSON_ENCODING)
    add_fuzzer(fuzz_json_decode fuzz_json_decode.cc)
    add_fuzzer(fuzz_json_decode_encode fuzz_json_decode_encode.cc)
endif()

if(UA_ENABLE_XML_ENCODING)
    add_fuzzer(fuzz_xml_decode_encode fuzz_xml_decode_encode.cc)
endif()

# Run fuzzer on existing corpus to avoid regression
file(GLOB CORPUS_FILES ${PROJECT_SOURCE_DIR}/tests/fuzz/fuzz_binary_message_corpus/generated/*)
SET(CORPUS_CMDS "")
FOREACH(f ${CORPUS_FILES})
    LIST(APPEND CORPUS_CMDS COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_binary_message "${f}")
ENDFOREACH(f)


#file(GLOB CORPUS_FILES ${PROJECT_SOURCE_DIR}/deps/mdnsd/tests/fuzz/fuzz_mdns_message_corpus/*)
#FOREACH(f ${CORPUS_FILES})
#    LIST(APPEND CORPUS_CMDS COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_mdns_message "${f}")
#ENDFOREACH(f)

add_custom_target(run_fuzzer ${CORPUS_CMDS}
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS ${FUZZER_TARGETS}
                  ${MAYBE_USES_TERMINAL})
