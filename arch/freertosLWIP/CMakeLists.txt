SET(SOURCE_GROUP ${SOURCE_GROUP}\\freertosLWIP)

ua_add_architecture("freertosLWIP")

list (FIND UA_AMALGAMATION_ARCHITECUTRES "freertosLWIP" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "freertosLWIP")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "freertosLWIP")

        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

        SET(UA_FREERTOS_INCLUDES "" CACHE STRING "Folders to include from the freeRTOS OS")
        ua_include_directories(${UA_FREERTOS_INCLUDES})

        ua_architecture_remove_definitions(-Wconversion )
        ua_architecture_add_definitions(-mthumb -mcpu=cortex-m3) #CPU dependant
        ua_architecture_add_definitions(-ffunction-sections -fdata-sections -fstack-usage -Wall -specs=nano.specs
                     -fno-exceptions -Wno-unused-variable -Wno-format -Wno-format-security -Wno-format-nonliteral)

        set(UA_FREERTOS_HOSTNAME "freertosLWIP-host" CACHE STRING "Define a custom hostname/ip for the endpoint.")
        ua_architecture_add_definitions(-DUA_FREERTOS_HOSTNAME="${UA_FREERTOS_HOSTNAME}")
    endif()
endif()
