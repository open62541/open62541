SET(SOURCE_GROUP ${SOURCE_GROUP}\\freertos)

ua_add_architecture("freertos")

if("${UA_ARCHITECTURE}" STREQUAL "freertos")

    ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    
    SET(UA_FREERTOS_INCLUDES "" CACHE STRING "Folders to include from the freeRTOS OS")
    ua_include_directories(${UA_FREERTOS_INCLUDES})
    
    ua_architecture_remove_definitions(-fPIC -Wconversion )
    ua_architecture_add_definitions(-DUA_FREERTOS -mcpu=cortex-m3 -mthumb -g -Wall -O0 -specs=nano.specs 
                -ffunction-sections -fdata-sections  -fno-exceptions -fstack-usage -Wno-unused-variable -Wno-format -Wno-format-security -Wno-format-nonliteral)
    ua_architecture_append_to_library(c m stdc++ supc++)

endif()