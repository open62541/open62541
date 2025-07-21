#check environment variable
if(NOT "$ENV{MBEDTLS_FOLDER_INCLUDE}" STREQUAL "")
    set(MBEDTLS_FOLDER_INCLUDE "$ENV{MBEDTLS_FOLDER_INCLUDE}")
endif()
if(NOT "$ENV{MBEDTLS_FOLDER_LIBRARY}" STREQUAL "")
    set(MBEDTLS_FOLDER_LIBRARY "$ENV{MBEDTLS_FOLDER_LIBRARY}")
endif()

find_path(MBEDTLS_INCLUDE_DIRS mbedtls/ssl.h HINTS ${MBEDTLS_FOLDER_INCLUDE})

# Handle library finding - respect user-provided paths first
if(UA_BUILD_OSS_FUZZ)
    # OSS-Fuzz builds require static linking of libraries
    # First try user-defined paths (via env), then fallback to known system path
    if(NOT DEFINED MBEDTLS_LIBRARY)
        find_library(MBEDTLS_LIBRARY
            NAMES libmbedtls.a mbedtls
            HINTS ${MBEDTLS_FOLDER_LIBRARY} /usr/lib/x86_64-linux-gnu
        )
    endif()
    if(NOT DEFINED MBEDX509_LIBRARY)
        find_library(MBEDX509_LIBRARY
            NAMES libmbedx509.a mbedx509
            HINTS ${MBEDTLS_FOLDER_LIBRARY} /usr/lib/x86_64-linux-gnu
        )
    endif()
    if(NOT DEFINED MBEDCRYPTO_LIBRARY)
        find_library(MBEDCRYPTO_LIBRARY
            NAMES libmbedcrypto.a mbedcrypto
            HINTS ${MBEDTLS_FOLDER_LIBRARY} /usr/lib/x86_64-linux-gnu
        )
    endif()
else()
    # For non-OSS-Fuzz builds, dynamic libraries are allowed
    # Respect user-defined path first, fallback to system discovery
    if(NOT DEFINED MBEDTLS_LIBRARY)
        find_library(MBEDTLS_LIBRARY mbedtls HINTS ${MBEDTLS_FOLDER_LIBRARY})
    endif()
    if(NOT DEFINED MBEDX509_LIBRARY)
        find_library(MBEDX509_LIBRARY mbedx509 HINTS ${MBEDTLS_FOLDER_LIBRARY})
    endif()
    if(NOT DEFINED MBEDCRYPTO_LIBRARY)
        find_library(MBEDCRYPTO_LIBRARY mbedcrypto HINTS ${MBEDTLS_FOLDER_LIBRARY})
    endif()
endif()

set(MBEDTLS_LIBRARIES ${MBEDTLS_LIBRARY} ${MBEDX509_LIBRARY} ${MBEDCRYPTO_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG MBEDTLS_INCLUDE_DIRS
                                  MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)

mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)
