# Toolchain for 32-bit WebAssembly with WASI based libc
#
# https://github.com/WebAssembly/wasi-sdk
#
# The WASI sysroot must be provided in the environment variable WASI_SYSROOT.

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(triple ${CMAKE_SYSTEM_PROCESSOR}-wasi)

set(CMAKE_SYSROOT $ENV{WASI_SYSROOT})

set(CMAKE_EXECUTABLE_SUFFIX_C ".wasm")
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_C_FLAGS_INIT "-D__wasi__")

set(CMAKE_EXECUTABLE_SUFFIX_CXX ".wasm")
set(CMAKE_CXX_COMPILER clang)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_CXX_FLAGS_INIT "-D__wasi__")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

