list(APPEND ua_architecture_sources ${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
list(APPEND ua_architecture_headers ${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture.h)

# Bring changes back to the parent scope
set(ua_architecture_sources ${ua_architecture_sources} PARENT_SCOPE)
set(ua_architecture_headers ${ua_architecture_headers} PARENT_SCOPE)
