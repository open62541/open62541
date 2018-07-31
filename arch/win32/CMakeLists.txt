# Add architecture-specific sources
list(APPEND ua_architecture_sources ${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
list(APPEND ua_architecture_headers ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture.h)

# Add libraries
if("${UA_ARCHITECTURE}" STREQUAL "WIN32")
    list(APPEND open62541_LIBRARIES ws2_32)
    if(UA_ENABLE_DISCOVERY_MULTICAST OR UA_ENABLE_PUBSUB)
        list(APPEND open62541_LIBRARIES iphlpapi)
    endif()

    if(MSVC)
        # Compiler warnings, error on warning
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /WX /w44996" PARENT_SCOPE)

        if(UA_MSVC_FORCE_STATIC_CRT AND NOT BUILD_SHARED_LIBS)
            set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS
                CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
            foreach(CompilerFlag ${CompilerFlags})
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
                set(${CompilerFlag} ${${CompilerFlag}} PARENT_SCOPE)
            endforeach()
        endif()
    endif()
endif()

# Bring changes back to the parent scope
set(ua_architecture_sources ${ua_architecture_sources} PARENT_SCOPE)
set(ua_architecture_headers ${ua_architecture_headers} PARENT_SCOPE)
set(open62541_LIBRARIES ${open62541_LIBRARIES} PARENT_SCOPE)
