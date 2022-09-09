set(SOURCE_GROUP ${SOURCE_GROUP}\\wasm)
    
ua_add_architecture("wasm")

list (FIND UA_AMALGAMATION_ARCHITECTURES "wasm" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "wasm")
    ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
endif()
