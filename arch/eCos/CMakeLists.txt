SET(SOURCE_GROUP ${SOURCE_GROUP}\\eCos)

ua_add_architecture("eCos")


list (FIND UA_AMALGAMATION_ARCHITECUTRES "eCos" _index)
if (${_index} GREATER -1 OR "${UA_ARCHITECTURE}" STREQUAL "eCos")

    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)

    if("${UA_ARCHITECTURE}" STREQUAL "eCos")

        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

        ua_architecture_remove_definitions(-Wpedantic -Wno-static-in-inline -Wc++-compat)
        ua_architecture_add_definitions(-Wno-sign-conversion -fdata-sections -fno-exceptions -ffunction-sections -DSYSTEM_BUS_WIDTH=32  -mhw-mul -mno-hw-mulx -mhw-div -Wa,-relax-all -W -Wall -Wextra -D_KERNEL -D__ECOS -nostdlib -Wl,--gc-sections -Wl,-static)

        ua_architecture_append_to_library("-L${UA_ECOS_INCLUDES}/lib -Wl,-T,${UA_ECOS_INCLUDES}/lib/target.ld -nostartfiles") #for examples

        SET(UA_ECOS_INCLUDES "" CACHE STRING "Folders to include from the eCos OS")

        ua_include_directories(${UA_ECOS_INCLUDES}/include)
        
        set(UA_ECOS_HOSTNAME "eCos-host" CACHE STRING "Define a custom hostname/ip for the endpoint.")
        ua_architecture_add_definitions(-DUA_ECOS_HOSTNAME="${UA_ECOS_HOSTNAME}")
    endif()

endif()
