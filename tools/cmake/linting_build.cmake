if(UA_ENABLE_STATIC_ANALYZER STREQUAL MINIMAL OR UA_ENABLE_STATIC_ANALYZER STREQUAL REDUCED OR UA_ENABLE_STATIC_ANALYZER STREQUAL FULL)
    # cpplint just gives warnings about coding style
    find_program(CPPLINT_EXE NAMES "cpplint")
    if(CPPLINT_EXE)
        set(CMAKE_C_CPPLINT "${CPPLINT_EXE};--quiet")
        set(CMAKE_CXX_CPPLINT "${CPPLINT_EXE};--quiet")
    endif()
endif()
if(UA_ENABLE_STATIC_ANALYZER STREQUAL REDUCED OR UA_ENABLE_STATIC_ANALYZER STREQUAL FULL)
    # clang-tidy has certain warnings as errors
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE};-p=compile_commands.json")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-p=compile_commands.json")
    endif()
elseif(UA_ENABLE_STATIC_ANALYZER STREQUAL FULL)
    # cppcheck provides just warnings but checks "all" (for now) - huge CPU impact
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        set(CMAKE_C_CPPCHECK "${CPPCHECK_EXE};--project=compile_commands.json;--enable=all;--inconclusive;--inline-suppr;\
--suppressions-list=${PROJECT_SOURCE_DIR}/cppcheck-suppressions.txt;-D__GNUC__;-i ${PROJECT_SOURCE_DIR}/build")
        set(CMAKE_CXX_CPPCHECK "${CPPCHECK_EXE};--project=compile_commands.json;--enable=all;--inconclusive;--inline-suppr;\
--suppressions-list=${PROJECT_SOURCE_DIR}/cppcheck-suppressions.txt;-D__GNUC__;-i ${PROJECT_SOURCE_DIR}/build")
    endif()

    # "include what you use" requires additional configuration - ignore for now
    find_program(IWYU_EXE NAMES "iwyu")
    if(IWYU_EXE)
        #set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}")
        #set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}")
    endif()
endif()

