# Check environment variables
if("$ENV{MBEDTLS_FOLDER_INCLUDE}")
    set(MBEDTLS_FOLDER_INCLUDE "$ENV{MBEDTLS_FOLDER_INCLUDE}")
endif()
if("$ENV{MBEDTLS_FOLDER_LIBRARY}")
    set(MBEDTLS_FOLDER_LIBRARY "$ENV{MBEDTLS_FOLDER_LIBRARY}")
endif()

# Find include directory
find_path(MBEDTLS_INCLUDE_DIRS mbedtls/ssl.h HINTS ${MBEDTLS_FOLDER_INCLUDE})

# Handle library finding - respect user-provided paths first
if(UA_BUILD_OSS_FUZZ)
    # oss-fuzz requires static linking of libraries
    # Only use default paths if user didn't specify custom ones
    if(NOT MBEDTLS_LIBRARY)
        set(MBEDTLS_LIBRARY /usr/lib/x86_64-linux-gnu/libmbedtls.a)
    endif()
    if(NOT MBEDX509_LIBRARY)
        set(MBEDX509_LIBRARY /usr/lib/x86_64-linux-gnu/libmbedx509.a)
    endif()
    if(NOT MBEDCRYPTO_LIBRARY)
        set(MBEDCRYPTO_LIBRARY /usr/lib/x86_64-linux-gnu/libmbedcrypto.a)
    endif()
else()
    # For non-OSS_FUZZ builds, use find_library if not already set
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

set(MBEDTLS_LIBRARIES ${MBEDTLS_LIBRARY} ${MBEDX509_LIBRARY} ${MBEDCRYPTO_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG MBEDTLS_INCLUDE_DIRS
                                  MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)

mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)
