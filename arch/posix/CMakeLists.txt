# Add architecture-specific sources
list(APPEND ua_architecture_sources ${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
list(APPEND ua_architecture_headers ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture.h)

if("${UA_ARCHITECTURE}" STREQUAL "POSIX")
    # Compiler Flags
    if(NOT UA_COMPILE_AS_CXX AND (CMAKE_COMPILER_IS_GNUCC OR "x${CMAKE_C_COMPILER_ID}" STREQUAL "xClang"))
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
            -std=c99 -pipe \
            -Wall -Wextra -Werror -Wpedantic \
            -Wno-static-in-inline \
            -Wno-overlength-strings \
            -Wno-unused-parameter \
            -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls \
            -Wformat -Wformat-security -Wformat-nonliteral \
            -Wuninitialized -Winit-self \
            -Wcast-qual \
            -Wstrict-overflow \
            -Wnested-externs \
            -Wmultichar \
            -Wundef \
            -Wc++-compat")

        if(UA_ENABLE_AMALGAMATION)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
        endif()

        if(NOT CYGWIN AND NOT QNXNTO AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow -Wconversion -fvisibility=hidden -fPIC")
        endif()
    endif()

    # MinSizeRel Settings
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
             -ffunction-sections -fdata-sections -fno-stack-protector -fno-unwind-tables \
             -fno-asynchronous-unwind-tables -fno-math-errno -fmerge-all-constants -fno-ident")

        if(NOT OS9)
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -s")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
        endif()

        if(APPLE)
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-dead_strip")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
        else()
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--gc-sections")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
        endif()

        if(NOT WIN32 AND NOT CYGWIN AND NOT APPLE)
            # these settings reduce the binary size by ~2kb
            set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} \
                -Wl,-z,norelro -Wl,--hash-style=gnu -Wl,--build-id=none")
        endif()
    endif()

    if(APPLE)
        set(CMAKE_MACOSX_RPATH 1 PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DARWIN_C_SOURCE=1")
    endif()

    # Set external library dependencies
    if(QNXNTO)
        list(APPEND open62541_LIBRARIES socket c stdc++)
    elseif(OS9)
        list(APPEND open62541_LIBRARIES netdb ndblib socket)
    else()
        list(APPEND open62541_LIBRARIES m)
        if(UA_ENABLE_MULTITHREADING OR UA_BUILD_UNIT_TESTS)
            list(APPEND open62541_LIBRARIES pthread)
        endif()
        if(NOT APPLE AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
            list(APPEND open62541_LIBRARIES rt)
        endif()
    endif()
endif()

# Bring changes back to the parent scope
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} PARENT_SCOPE)
set(CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS} PARENT_SCOPE)
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} PARENT_SCOPE)
set(ua_architecture_sources ${ua_architecture_sources} PARENT_SCOPE)
set(ua_architecture_headers ${ua_architecture_headers} PARENT_SCOPE)
set(open62541_LIBRARIES ${open62541_LIBRARIES} PARENT_SCOPE)
