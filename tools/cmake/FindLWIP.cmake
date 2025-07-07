# FindLWIP.cmake

# Look for the lwip headers and libraries
find_path(LWIP_INCLUDE_DIRS
          NAMES lwip/init.h
          PATHS /usr/include/lwip
                /usr/include/lwip/lwip
                /usr/local/include/lwip
                /usr/local/include/lwip/lwip
)

find_library(LWIP_LIBRARIES
             NAMES lwip
             PATHS /usr/lib/x86_64-linux-gnu/
                   /usr/local/lib/
)

# Mark the variables as required
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LWIP DEFAULT_MSG LWIP_INCLUDE_DIRS LWIP_LIBRARIES)

# Provide the results to the calling script
if(LWIP_FOUND)
    set(LWIP_INCLUDE_DIRS ${LWIP_INCLUDE_DIRS})
    set(LWIP_LIBRARIES ${LWIP_LIBRARIES})
    mark_as_advanced(LWIP_INCLUDE_DIRS LWIP_LIBRARIES)
else()
    message(FATAL_ERROR "Could not find LWIP")
endif()

mark_as_advanced(LWIP_INCLUDE_DIRS LWIP_LIBRARIES)
