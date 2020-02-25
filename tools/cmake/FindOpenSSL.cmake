if("${UA_ARCHITECTURE}" STREQUAL "vxworks")
    set(OPENSSL_LIBRARIES crypto)
else()
    #check environment variable
    if("$ENV{OENSSL_FOLDER_INCLUDE}")
        set(OENSSL_FOLDER_INCLUDE "$ENV{OENSSL_FOLDER_INCLUDE}")
    endif()
    if("$ENV{OPENSSL_FOLDER_LIBRARY}")
        set(OPENSSL_FOLDER_LIBRARY "$ENV{OPENSSL_FOLDER_LIBRARY}")
    endif()
    find_path(OPENSSL_INCLUDE_DIRS openssl/evp.h HINTS ${OENSSL_FOLDER_INCLUDE})

    if(UA_BUILD_OSS_FUZZ)
        # oss-fuzz requires static linking of libraries
        set(OPENSSLCRYPTO_LIBRARY /usr/lib/x86_64-linux-gnu/libcrypto.a)
    else()
        find_library(OPENSSLCRYPTO_LIBRARY crypto HINTS ${OPENSSL_FOLDER_LIBRARY})
    endif()

    add_library(crypto UNKNOWN IMPORTED)
    set_property(TARGET crypto PROPERTY IMPORTED_LOCATION "${OPENSSLCRYPTO_LIBRARY}")
    set(OPENSSL_LIBRARIES crypto)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OPENSSL DEFAULT_MSG
             OPENSSL_INCLUDE_DIRS OPENSSLCRYPTO_LIBRARY)
    mark_as_advanced(OPENSSL_INCLUDE_DIRS OPENSSLCRYPTO_LIBRARY)
endif()