# FindMbedTLS.cmake
# 
# This module finds the MbedTLS library and its dependencies.
# It sets the following variables:
#   MBEDTLS_FOUND         - True if MbedTLS is found
#   MBEDTLS_INCLUDE_DIRS  - Include directories for MbedTLS
#   MBEDTLS_LIBRARIES     - Link these to use MbedTLS
#   MBEDTLS_LIBRARY       - MbedTLS library
#   MBEDX509_LIBRARY      - MbedX509 library  
#   MBEDCRYPTO_LIBRARY    - MbedCrypto library
#
# Users can set the following variables to guide the search:
#   MBEDTLS_FOLDER_INCLUDE - Directory containing mbedtls headers
#   MBEDTLS_FOLDER_LIBRARY - Directory containing mbedtls libraries
#   MBEDTLS_LIBRARY        - Full path to mbedtls library
#   MBEDX509_LIBRARY       - Full path to mbedx509 library
#   MBEDCRYPTO_LIBRARY     - Full path to mbedcrypto library

# Check environment variables for library paths
if("$ENV{MBEDTLS_FOLDER_INCLUDE}")
    set(MBEDTLS_FOLDER_INCLUDE "$ENV{MBEDTLS_FOLDER_INCLUDE}")
endif()
if("$ENV{MBEDTLS_FOLDER_LIBRARY}")
    set(MBEDTLS_FOLDER_LIBRARY "$ENV{MBEDTLS_FOLDER_LIBRARY}")
endif()

# Find include directory
find_path(MBEDTLS_INCLUDE_DIRS mbedtls/ssl.h HINTS ${MBEDTLS_FOLDER_INCLUDE})

# Library discovery - only search if not already defined by user
if(NOT DEFINED MBEDTLS_LIBRARY)
    find_library(MBEDTLS_LIBRARY mbedtls HINTS ${MBEDTLS_FOLDER_LIBRARY})
endif()
if(NOT DEFINED MBEDX509_LIBRARY)
    find_library(MBEDX509_LIBRARY mbedx509 HINTS ${MBEDTLS_FOLDER_LIBRARY})
endif()
if(NOT DEFINED MBEDCRYPTO_LIBRARY)
    find_library(MBEDCRYPTO_LIBRARY mbedcrypto HINTS ${MBEDTLS_FOLDER_LIBRARY})
endif()

# Set the libraries variable for convenience
set(MBEDTLS_LIBRARIES ${MBEDTLS_LIBRARY} ${MBEDX509_LIBRARY} ${MBEDCRYPTO_LIBRARY})

# Standard CMake package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG MBEDTLS_INCLUDE_DIRS
                                  MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)

# Mark variables as advanced to hide them from basic CMake GUI
mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)
