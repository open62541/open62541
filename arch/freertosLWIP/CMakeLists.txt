list(APPEND ua_architecture_sources ${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
list(APPEND ua_architecture_headers ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture.h)

if("${UA_ARCHITECTURE}" STREQUAL "FREERTOSLWIP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPEN62541_FEERTOS_USE_OWN_MEM" PARENT_SCOPE)
    list(INSERT ua_architecture_headers 0
                ${CMAKE_CURRENT_SOURCE_DIR}/../common/freeRTOS62541.h
                ${CMAKE_CURRENT_SOURCE_DIR}/../common/lwip62541.h) 
    endif()
endif()

# Bring changes back to the parent scope
set(ua_architecture_sources ${ua_architecture_sources} PARENT_SCOPE)
set(ua_architecture_headers ${ua_architecture_headers} PARENT_SCOPE)
