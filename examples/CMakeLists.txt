include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR})

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

#############
# Tutorials #
#############

add_executable(tutorial_datatypes tutorial_datatypes.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_datatypes open625451_amalgamation)
target_link_libraries(tutorial_datatypes ${LIBS})

add_executable(tutorial_server_firststeps tutorial_server_firststeps.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_server_firststeps open625451_amalgamation)
target_link_libraries(tutorial_server_firststeps ${LIBS})

add_executable(tutorial_server_variable tutorial_server_variable.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_server_variable open625451_amalgamation)
target_link_libraries(tutorial_server_variable ${LIBS})

add_executable(tutorial_server_datasource tutorial_server_datasource.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_server_datasource open625451_amalgamation)
target_link_libraries(tutorial_server_datasource ${LIBS})

add_executable(tutorial_server_variabletype tutorial_server_variabletype.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_server_variabletype open625451_amalgamation)
target_link_libraries(tutorial_server_variabletype ${LIBS})

add_executable(tutorial_server_object tutorial_server_object.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_server_object open625451_amalgamation)
target_link_libraries(tutorial_server_object ${LIBS})

if(UA_ENABLE_METHODCALLS)
  add_executable(tutorial_server_method tutorial_server_method.c $<TARGET_OBJECTS:open62541-object>)
  add_dependencies(tutorial_server_method open625451_amalgamation)
  target_link_libraries(tutorial_server_method ${LIBS})
endif()

add_executable(tutorial_client_firststeps tutorial_client_firststeps.c $<TARGET_OBJECTS:open62541-object>)
add_dependencies(tutorial_client_firststeps open625451_amalgamation)
target_link_libraries(tutorial_client_firststeps ${LIBS})

##################
# Example Server #
##################

add_executable(server server.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server ${LIBS})

if(UA_ENABLE_NONSTANDARD_UDP)
  add_executable(server_udp server_udp.c $<TARGET_OBJECTS:open62541-object> ${PROJECT_SOURCE_DIR}/plugins/ua_network_udp.c)
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

add_executable(server_mainloop server_mainloop.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_mainloop ${LIBS})

add_executable(server_instantiation server_instantiation.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_instantiation ${LIBS})

add_executable(server_repeated_job server_repeated_job.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_repeated_job ${LIBS})

add_executable(server_inheritance server_inheritance.c $<TARGET_OBJECTS:open62541-object>)
target_link_libraries(server_inheritance ${LIBS})

if(UA_BUILD_EXAMPLES_NODESET_COMPILER)
  add_executable(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/nodeset.c $<TARGET_OBJECTS:open62541-object>)
  target_link_libraries(server_nodeset ${LIBS})
  target_include_directories(server_nodeset PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
endif()

if(UA_BUILD_SELFSIGNED_CERTIFICATE)
  find_package(OpenSSL REQUIRED)
  add_custom_command(OUTPUT server_cert.der ca.crt
                     COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${CMAKE_CURRENT_BINARY_DIR}
                     DEPENDS ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py
                             ${PROJECT_SOURCE_DIR}/tools/certs/localhost.cnf)
                             
  add_custom_target(selfsigned ALL DEPENDS server_cert.der ca.crt)

  add_executable(server_certificate server_certificate.c $<TARGET_OBJECTS:open62541-object> server_cert.der ca.crt)
  target_link_libraries(server_certificate ${LIBS})
endif()

add_executable(server_readspeed server_readspeed.c $<TARGET_OBJECTS:open62541-object>)
target_include_directories(server_readspeed PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
target_link_libraries(server_readspeed ${LIBS})
