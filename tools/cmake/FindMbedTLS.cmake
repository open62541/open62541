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

# Standard CMake package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG 
    MBEDTLS_INCLUDE_DIRS
    MBEDTLS_LIBRARY 
    MBEDX509_LIBRARY 
    MBEDCRYPTO_LIBRARY
)

mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)
