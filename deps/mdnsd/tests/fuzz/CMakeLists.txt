
if (NOT MDNSD_BUILD_OSS_FUZZ)
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

list(APPEND LIBS libmdnsd)

set(FUZZER_TARGETS)

macro(add_fuzzer FUZZER_NAME FUZZER_SOURCE)
    add_executable(${FUZZER_NAME} ${FUZZER_SOURCE} ${ARGN})
    target_link_libraries(${FUZZER_NAME} ${LIBS})
    list(APPEND FUZZER_TARGETS ${FUZZER_NAME})
endmacro()

# Add new fuzzers here
add_fuzzer(fuzz_mdns_message fuzz_mdns_message.cc)
add_fuzzer(fuzz_mdns_xht fuzz_mdns_xht.cc)


SET(CORPUS_CMDS "")
FOREACH(f ${CORPUS_FILES})
    LIST(APPEND CORPUS_CMDS COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fuzz_mdns_message "${f}")
ENDFOREACH(f)

add_custom_target(run_fuzzer ${CORPUS_CMDS}
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS ${FUZZER_TARGETS}
                  ${MAYBE_USES_TERMINAL})
