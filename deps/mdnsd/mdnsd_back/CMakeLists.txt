cmake_minimum_required(VERSION 2.8)

project(mdnsd)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/cmake")

option(MDNSD_BUILD_OSS_FUZZ "Special build switch used in oss-fuzz" OFF)
mark_as_advanced(MDNSD_BUILD_OSS_FUZZ)

option(MDNSD_BUILD_FUZZING "Build the fuzzing executables" OFF)
mark_as_advanced(MDNSD_BUILD_FUZZING)
if(MDNSD_BUILD_FUZZING)
    # oss-fuzz already defines this by default
    add_definitions(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
endif()

if(MDNSD_BUILD_FUZZING_CORPUS)
    add_definitions(-DMDNSD_DEBUG_DUMP_PKGS_FILE)
endif()

# Debug
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER_CASE)
if(BUILD_TYPE_LOWER_CASE STREQUAL "debug" AND UNIX)
    if("x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang")
        # Add default sanitizer settings when using clang and Debug build.
        # This allows e.g. CLion to find memory locations for SegFaults
        message(STATUS "Sanitizer enabled")
        set(SANITIZER_FLAGS "-g -fno-omit-frame-pointer -gline-tables-only -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -fsanitize=leak -fsanitize=undefined")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    endif()
endif()

add_subdirectory(libmdnsd)

#option(MDNSD_BUILD_UNIT_TESTS "Build the unit tests" OFF)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR}/libmdnsd)

if(MDNSD_BUILD_FUZZING OR MDNSD_BUILD_OSS_FUZZ)
    add_subdirectory(tests/fuzz)
endif()

add_executable(mdnsd mdnsd.c)
target_link_libraries(mdnsd libmdnsd)
if(WIN32 OR MINGW)
  target_link_libraries(mdnsd wsock32 ws2_32)
endif()

add_executable(mquery mquery.c)
target_link_libraries(mquery libmdnsd)
if(WIN32 OR MINGW)
  target_link_libraries(mquery wsock32 ws2_32)
endif()


#if(MDNSD_BUILD_UNIT_TESTS)
#  enable_testing()
#  add_subdirectory(tests)
#endif()
