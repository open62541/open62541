cmake_minimum_required(VERSION 2.8)

project(mdnsd C CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/cmake")

include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

option(MDNSD_BUILD_OSS_FUZZ "Special build switch used in oss-fuzz" OFF)
mark_as_advanced(MDNSD_BUILD_OSS_FUZZ)

option(MDNSD_BUILD_FUZZING "Build the fuzzing executables" OFF)
mark_as_advanced(MDNSD_BUILD_FUZZING)
if(MDNSD_BUILD_FUZZING)
    # oss-fuzz already defines this by default
    add_definitions(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
endif()

set(BUILD_SHARED_LIBS_DEFAULT ON)
if(MDNSD_BUILD_OSS_FUZZ)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)
endif()
option(BUILD_SHARED_LIBS "Build libmdnsd as a shared library" ${BUILD_SHARED_LIBS_DEFAULT})

if(MDNSD_BUILD_FUZZING_CORPUS)
    add_definitions(-DMDNSD_DEBUG_DUMP_PKGS_FILE)
endif()

option(MDNSD_ENABLE_SANITIZERS "enable sanitizers for mdnsd ON)" ON)

# Debug
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER_CASE)
if(MDNSD_ENABLE_SANITIZERS AND BUILD_TYPE_LOWER_CASE STREQUAL "debug" AND UNIX)
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

if(MDNSD_BUILD_FUZZING OR MDNSD_BUILD_OSS_FUZZ)
    add_subdirectory(tests/fuzz)
endif()

add_executable(mdnsd mdnsd.c)
target_link_libraries(mdnsd PRIVATE libmdnsd)

add_executable(mquery mquery.c)
target_link_libraries(mquery PRIVATE libmdnsd)

include(CMakePackageConfigHelpers)
configure_package_config_file(mdnsdConfig.cmake.in mdnsdConfig.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

install(
    TARGETS mdnsd mquery
    EXPORT mdnsd
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
)
install(EXPORT mdnsd
    DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
    FILE "mdnsdConfigTargets.cmake"
)
install(FILES "${PROJECT_BINARY_DIR}/mdnsdConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
)
