SET(SOURCE_GROUP ${SOURCE_GROUP}\\freertos)

ua_add_architecture("freertos")

list (FIND UA_AMALGAMATION_ARCHITECUTRES "freertos" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "freertos")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "freertos")

        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

        SET(UA_FREERTOS_INCLUDES "" CACHE STRING "Folders to include from the freeRTOS OS")
        ua_include_directories(${UA_FREERTOS_INCLUDES})

        ua_architecture_remove_definitions(-Wconversion )
        ua_architecture_add_definitions(-DUA_FREERTOS -mcpu=cortex-m3 -mthumb -g -Wall -O0 -specs=nano.specs
                    -ffunction-sections -fdata-sections  -fno-exceptions -fstack-usage -Wno-unused-variable -Wno-format -Wno-format-security -Wno-format-nonliteral)
        ua_architecture_append_to_library(c m stdc++ supc++)

        set(UA_FREERTOS_HOSTNAME "freertos-host" CACHE STRING "Define a custom hostname/ip for the endpoint.")
        ua_architecture_add_definitions(-DUA_FREERTOS_HOSTNAME="${UA_FREERTOS_HOSTNAME}")
    endif()
endif()
