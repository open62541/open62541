list(APPEND ua_architecture_sources ${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
list(APPEND ua_architecture_headers ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture.h)

if("${UA_ARCHITECTURE}" STREQUAL "ECOS")
    set(UA_ECOS_HOSTNAME "eCos-host" CACHE STRING "Define a custom hostname/ip for the endpoint." PARENT_SCOPE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUA_ECOS_HOSTNAME=${UA_ECOS_HOSTNAME}" PARENT_SCOPE)
endif()

# Bring changes back to the parent scope
set(ua_architecture_sources ${ua_architecture_sources} PARENT_SCOPE)
set(ua_architecture_headers ${ua_architecture_headers} PARENT_SCOPE)
