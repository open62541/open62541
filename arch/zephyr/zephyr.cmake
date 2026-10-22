# Generate the Zephyr architecture configuration header.

set(UA_ARCHITECTURE_INCLUDE "arch/zephyr.h")
configure_file("${CMAKE_CURRENT_LIST_DIR}/zephyr.h.in"
               "${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_ARCHITECTURE_INCLUDE}")

# Encryption with the mbedTLS of the Zephyr build: find_package(MbedTLS) takes
# its headers and library targets instead of searching the host. Zephyr 4.4
# moved to mbedTLS 4 with TF-PSA-Crypto (CONFIG_MBEDTLS_VERSION_4_x), whose
# libraries have other names.
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
