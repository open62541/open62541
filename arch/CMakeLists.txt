set(available_architectures posix win32 freertos vxworks eCos wec7)

list(APPEND current_architecture_sources ${CMAKE_CURRENT_SOURCE_DIR}/network_tcp.c)

#########
# POSIX #
#########

list(FIND UA_AMALGAMATION_ARCHITECTURES "posix" _index)
if(${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "posix")
    list(APPEND current_architecture_sources
                ${CMAKE_CURRENT_SOURCE_DIR}/posix/ua_clock.c
                ${CMAKE_CURRENT_SOURCE_DIR}/posix/ua_architecture_functions.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/posix/arch_posix.h
                   ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_posix.h)
    list(APPEND current_architecture_headers
                ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_posix.h)
    if("${UA_ARCHITECTURE}" STREQUAL "posix")
        if(NOT CYGWIN AND NOT QNXNTO AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
          set(current_architecture_definitions -Wshadow -Wconversion -fvisibility=hidden -fPIC)
        endif()
        if(QNXNTO)
          set(current_architecture_libraries socket c stdc++)
        elseif(OS9)
          set(current_architecture_libraries netdb ndblib socket)
        else()
          set(current_architecture_libraries m)
          list(APPEND current_architecture_libraries pthread)
          if(NOT APPLE AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD"))
            list(APPEND current_architecture_libraries rt)
          endif()
        endif()
    endif()
endif()

#########
# WIN32 #
#########

list(FIND UA_AMALGAMATION_ARCHITECTURES "win32" _index)
if(${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "win32")
    list(APPEND current_architecture_sources
                ${CMAKE_CURRENT_SOURCE_DIR}/win32/ua_clock.c
                ${CMAKE_CURRENT_SOURCE_DIR}/win32/ua_architecture_functions.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/win32/arch_win32.h
                   ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_win32.h)
    list(APPEND current_architecture_headers
                ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_win32.h)
    if("${UA_ARCHITECTURE}" STREQUAL "win32")
      set(current_architecture_libraries ws2_32 iphlpapi)
    endif()
endif()

############
# FREERTOS #
############

list(FIND UA_AMALGAMATION_ARCHITECTURES "freertos" _index)
if(${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "freertos")
    list(APPEND current_architecture_sources
                ${CMAKE_CURRENT_SOURCE_DIR}/freertos/ua_clock.c
                ${CMAKE_CURRENT_SOURCE_DIR}/freertos/ua_architecture_functions.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/freertos/arch_freertos.h
                   ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_freertos.h)
    list(APPEND current_architecture_headers
                ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_freertos.h)
    if("${UA_ARCHITECTURE}" STREQUAL "freertos")
        set(UA_ARCH_FREERTOS_USE_OWN_MEMORY_FUNCTIONS OFF CACHE BOOL
            "Enable free/malloc/etc from own implementation")
        if(UA_ARCH_FREERTOS_USE_OWN_MEMORY_FUNCTIONS)
            set(current_architecture_definitions -DOPEN62541_FEERTOS_USE_OWN_MEM)
        endif()
    endif()
endif()

###########
# VXWORKS #
###########

list(FIND UA_AMALGAMATION_ARCHITECTURES "vxworks" _index)
if(${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "vxworks")
    list(APPEND current_architecture_sources
                ${CMAKE_CURRENT_SOURCE_DIR}/vxworks/ua_clock.c
                ${CMAKE_CURRENT_SOURCE_DIR}/vxworks/ua_architecture_functions.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vxworks/arch_vxworks.h
                   ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_vxworks.h)
    list(APPEND current_architecture_headers
                ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_vxworks.h)
endif()

########
# ECOS #
########

list(FIND UA_AMALGAMATION_ARCHITECTURES "eCos" _index)
if(${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "eCos")
    list(APPEND current_architecture_sources
                ${CMAKE_CURRENT_SOURCE_DIR}/ecos/ua_clock.c
                ${CMAKE_CURRENT_SOURCE_DIR}/ecos/ua_architecture_functions.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ecos/arch_ecos.h
                   ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_ecos.h)
    list(APPEND current_architecture_headers
                ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_ecos.h)
    if("${UA_ARCHITECTURE}" STREQUAL "eCos")
        set(UA_ECOS_HOSTNAME "eCos-host" CACHE STRING "Define a custom hostname/ip for the endpoint.")
        set(current_architecture_definitions -DUA_ECOS_HOSTNAME="${UA_ECOS_HOSTNAME}")
    endif()
endif()

########
# WEC7 #
########

list(FIND UA_AMALGAMATION_ARCHITECTURES "wec7" _index)
if(${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "wec7")
    list(APPEND current_architecture_sources
                ${CMAKE_CURRENT_SOURCE_DIR}/wec7/ua_clock.c
                ${CMAKE_CURRENT_SOURCE_DIR}/wec7/ua_architecture_functions.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wec7/arch_wec7.h
                   ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_wec7.h)
    list(APPEND current_architecture_headers
                ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_wec7.h)
    if("${UA_ARCHITECTURE}" STREQUAL "wec7")
        set(current_architecture_libraries ws2)
    endif()
endif()

# Add arch_common at the end
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/arch_common.h
               ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_common.h)
list(APPEND current_architecture_headers
            ${PROJECT_BINARY_DIR}/src_generated/open62541/arch_common.h)

# Propagate variables up to the parent scope
set(available_architectures ${available_architectures} PARENT_SCOPE)
set(current_architecture_sources ${current_architecture_sources} PARENT_SCOPE)
set(current_architecture_headers ${current_architecture_headers} PARENT_SCOPE)
set(current_architecture_definitions ${current_architecture_definitions} PARENT_SCOPE)
set(current_architecture_libraries ${current_architecture_libraries} PARENT_SCOPE)
