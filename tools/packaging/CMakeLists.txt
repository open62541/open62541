##########################
# Packaging (DEB/RPM)    #
##########################
# invoke via `make package`

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_PACKAGE_VENDOR "open62541 team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OPC UA implementation")
set(CPACK_PACKAGE_DESCRIPTION "open62541 is a C-based library (linking with C++ projects is possible) with all necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${OPEN62541_VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${OPEN62541_VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${OPEN62541_VER_PATCH}${OPEN62541_VER_LABEL}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "open62541 team") #required

include(CPack)
