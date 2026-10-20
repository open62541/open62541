# Generate the Zephyr architecture configuration header.

set(UA_ARCHITECTURE_INCLUDE "arch/zephyr.h")
configure_file("${CMAKE_CURRENT_LIST_DIR}/zephyr.h.in"
               "${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_ARCHITECTURE_INCLUDE}")
