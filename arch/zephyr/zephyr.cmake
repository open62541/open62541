# Generate the Zephyr architecture configuration header.

set(UA_ARCHITECTURE_INCLUDE "arch/zephyr.h")
set(UA_ZEPHYR_MALLOC_EXTERN ${CONFIG_OPEN62541_MALLOC_EXTERN})
configure_file("${CMAKE_CURRENT_LIST_DIR}/zephyr.h.in"
               "${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_ARCHITECTURE_INCLUDE}")

# Use the mbedTLS built by Zephyr: find_package(MbedTLS) takes these values
# instead of searching the host. The library targets differ between mbedTLS
# 3.x and 4.x.
if(UA_ENABLE_ENCRYPTION_MBEDTLS AND CONFIG_MBEDTLS_BUILTIN)
    set(MBEDTLS_INCLUDE_DIRS "${ZEPHYR_MBEDTLS_MODULE_DIR}/include" CACHE PATH "" FORCE)
    if(CONFIG_MBEDTLS_VERSION_4_x)
        set(MBEDTLS_LIBRARY "mbedtls" CACHE STRING "" FORCE)
        set(MBEDX509_LIBRARY "mbedx509" CACHE STRING "" FORCE)
        set(MBEDCRYPTO_LIBRARY "tfpsacrypto" CACHE STRING "" FORCE)
    else()
        set(MBEDTLS_LIBRARY "mbedTLSBase" CACHE STRING "" FORCE)
        set(MBEDX509_LIBRARY "mbedTLSX509" CACHE STRING "" FORCE)
        set(MBEDCRYPTO_LIBRARY "mbedTLSCrypto" CACHE STRING "" FORCE)
    endif()
endif()
