include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins)

if(UA_ENABLE_AMALGAMATION)
    include_directories(${PROJECT_BINARY_DIR}) # contain open62541.h
else()
    add_definitions(-DUA_NO_AMALGAMATION)
endif()

list(APPEND LIBS ${open62541_LIBRARIES})
if(NOT WIN32)
  list(APPEND LIBS pthread)
  if (NOT APPLE)
    list(APPEND LIBS rt)
  endif()
else()
  list(APPEND LIBS ws2_32)
endif()
if(UA_ENABLE_MULTITHREADING)
  list(APPEND LIBS urcu-cds urcu urcu-common)
endif(UA_ENABLE_MULTITHREADING)

##################
# Example Server #
##################

add_executable(server server.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server ${LIBS})

if(UA_ENABLE_NONSTANDARD_UDP)
  add_executable(server_udp server_udp.c $<TARGET_OBJECTS:open62541-object> ${PROJECT_SOURCE_DIR}/plugins/networklayer_udp.c)
  target_link_libraries(server_udp ${LIBS})
endif()

##################
# Example Client #
##################

add_executable(client client.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(client ${LIBS})

####################
# Feature Examples #
####################

add_executable(server_variable server_variable.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_variable ${LIBS})

add_executable(server_mainloop server_mainloop.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_mainloop ${LIBS})

add_executable(server_datasource server_datasource.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_datasource ${LIBS})

add_executable(server_firstSteps server_firstSteps.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_firstSteps ${LIBS})

add_executable(client_firstSteps client_firstSteps.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(client_firstSteps ${LIBS})

add_executable(server_repeated_job server_repeated_job.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_repeated_job ${LIBS})

if(UA_BUILD_EXAMPLES_NODESET_COMPILER)
  add_executable(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/nodeset.c $<TARGET_OBJECTS:open62541-object>)
  target_link_libraries(server_nodeset ${LIBS})
  target_include_directories(server_nodeset PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
endif()

if(UA_ENABLE_METHODCALLS)
  add_executable(server_method server_method.c $<TARGET_OBJECTS:open62541-object>)
  target_link_libraries(server_method ${LIBS})
endif()

add_executable(server_readspeed server_readspeed.c $<TARGET_OBJECTS:open62541-object>)
target_include_directories(server_readspeed PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
target_link_libraries(server_readspeed ${LIBS})
