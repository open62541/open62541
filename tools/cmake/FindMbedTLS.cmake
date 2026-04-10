# Check environment variables
if(NOT "$ENV{MBEDTLS_FOLDER_INCLUDE}" STREQUAL "")
    set(MBEDTLS_FOLDER_INCLUDE "$ENV{MBEDTLS_FOLDER_INCLUDE}")
endif()
if(NOT "$ENV{MBEDTLS_FOLDER_LIBRARY}" STREQUAL "")
    set(MBEDTLS_FOLDER_LIBRARY "$ENV{MBEDTLS_FOLDER_LIBRARY}")
endif()

# Find include directory
find_path(MBEDTLS_INCLUDE_DIRS mbedtls/ssl.h
    HINTS ${MBEDTLS_FOLDER_INCLUDE}
    PATHS /usr/include /usr/local/include
)

# OSS-Fuzz builds use static libraries
if(UA_BUILD_OSS_FUZZ)
    set(_OLD_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    
    if(NOT MBEDTLS_LIBRARY)
        find_library(MBEDTLS_LIBRARY
            NAMES mbedtls
            HINTS ${MBEDTLS_FOLDER_LIBRARY}
            PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib
        )
    endif()
    if(NOT MBEDX509_LIBRARY)
        find_library(MBEDX509_LIBRARY
            NAMES mbedx509
            HINTS ${MBEDTLS_FOLDER_LIBRARY}
            PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib
        )
    endif()
    if(NOT MBEDCRYPTO_LIBRARY)
        find_library(MBEDCRYPTO_LIBRARY
            NAMES mbedcrypto
            HINTS ${MBEDTLS_FOLDER_LIBRARY}
            PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib
        )
    endif()
    
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_OLD_SUFFIXES})
else()
    # Dynamic library fallback for non-OSS-Fuzz builds
    if(NOT MBEDTLS_LIBRARY)
        find_library(MBEDTLS_LIBRARY mbedtls HINTS ${MBEDTLS_FOLDER_LIBRARY})
    endif()
    if(NOT MBEDX509_LIBRARY)
        find_library(MBEDX509_LIBRARY mbedx509 HINTS ${MBEDTLS_FOLDER_LIBRARY})
    endif()
    if(NOT MBEDCRYPTO_LIBRARY)
        find_library(MBEDCRYPTO_LIBRARY mbedcrypto HINTS ${MBEDTLS_FOLDER_LIBRARY})
    endif()
endif()

# Set the libraries variable for convenience
set(MBEDTLS_LIBRARIES ${MBEDTLS_LIBRARY} ${MBEDX509_LIBRARY} ${MBEDCRYPTO_LIBRARY})

# Detect mbedTLS major version from header
if(MBEDTLS_INCLUDE_DIRS)
    foreach(_ver_hdr "mbedtls/build_info.h" "mbedtls/version.h")
        set(_ver_path "${MBEDTLS_INCLUDE_DIRS}/${_ver_hdr}")
        if(EXISTS "${_ver_path}")
            file(STRINGS "${_ver_path}" _ver_line
                 REGEX "^#[ \t]*define[ \t]+MBEDTLS_VERSION_NUMBER[ \t]+0x")
            if(_ver_line)
                string(REGEX REPLACE ".*0x([0-9a-fA-F]+).*" "\\1" _ver_hex "${_ver_line}")
                math(EXPR MBEDTLS_VERSION_MAJOR "0x${_ver_hex} >> 24")
                message(STATUS "mbedTLS major version: ${MBEDTLS_VERSION_MAJOR}")
                break()
            endif()
        endif()
    endforeach()
endif()

# Standard CMake package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG 
    MBEDTLS_INCLUDE_DIRS
    MBEDTLS_LIBRARY 
    MBEDX509_LIBRARY 
    MBEDCRYPTO_LIBRARY
)

mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)
