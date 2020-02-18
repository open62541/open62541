cmake_minimum_required(VERSION 2.8)

project(libmdnsd)

set(MDNSD_LOGLEVEL 300 CACHE STRING "Level at which logs shall be reported")

# Force compilation with as C++
option(MDNSD_COMPILE_AS_CXX "Force compilation with a C++ compiler" OFF)
mark_as_advanced(MDNSD_COMPILE_AS_CXX)

if(NOT MDNSD_COMPILE_AS_CXX AND (CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang"))
  add_definitions(-std=c99 -pipe
                  -Wall -Wextra -Werror
                  -Wno-unused-parameter # some methods may require unused arguments to cast to a method pointer
                  -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls
                  -Wformat -Wformat-security -Wformat-nonliteral
                  -Wuninitialized -Winit-self
                  -Wcast-qual -Wcast-align
                  -Wstrict-overflow
                  -Wnested-externs
                  -Wmultichar
                  -Wundef
                  -Wc++-compat)
  if(NOT MINGW AND NOT MSYS AND NOT MSVC)
    add_definitions(-Wpedantic)
  endif()
  if(NOT WIN32 AND NOT CYGWIN)
    add_definitions(-Wshadow -Wconversion -fvisibility=hidden -fPIC)
  endif()
endif()

option(MDNSD_ENABLE_COVERAGE "Enable gcov coverage" OFF)
if(MDNSD_ENABLE_COVERAGE)
  set(CMAKE_BUILD_TYPE DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# Building shared libs (dll, so)
set(MDNSD_DYNAMIC_LINKING OFF)
if(BUILD_SHARED_LIBS)
  set(MDNSD_DYNAMIC_LINKING ON)
endif()

configure_file("mdnsd_config.h.in" "${PROJECT_BINARY_DIR}/mdnsd_config.h")
include_directories(${PROJECT_BINARY_DIR})

set(LIBRARY_FILES mdnsd.c mdnsd.h 1035.c 1035.h sdtxt.c sdtxt.h xht.c xht.h)


if (MDNSD_BUILD_FUZZING_CORPUS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_definitions(-DMDNSD_CORPUS_OUTPUT_DIR="${PROJECT_BINARY_DIR}/corpus/custom")
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/corpus/custom)

    list(APPEND LIBRARY_FILES mdnsd_debug_dump_pkgs_file.c)
endif()

if(MDNSD_COMPILE_AS_CXX)
  set_source_files_properties(${LIBRARY_FILES} PROPERTIES LANGUAGE CXX)
endif()

if(MDNSD_BUILD_OSS_FUZZ)
    add_library(libmdnsd ${LIBRARY_FILES})
else()
    add_library(libmdnsd SHARED ${LIBRARY_FILES})
endif()

if(WIN32 OR MINGW)
  target_link_libraries(libmdnsd wsock32 ws2_32)
endif()

target_compile_definitions(libmdnsd PRIVATE -DMDNSD_DYNAMIC_LINKING_EXPORT)
