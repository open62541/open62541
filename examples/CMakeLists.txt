include_directories(${PROJECT_BINARY_DIR}
                    ${PROJECT_BINARY_DIR}/src_generated
                    ${PROJECT_SOURCE_DIR}/plugins)

#############
# Tutorials #
#############

add_executable(tutorial_datatypes tutorial_datatypes.c)
target_link_libraries(tutorial_datatypes open62541 ${open62541_LIBRARIES})

add_executable(tutorial_server_firststeps tutorial_server_firststeps.c)
target_link_libraries(tutorial_server_firststeps open62541 ${open62541_LIBRARIES})

add_executable(tutorial_server_variable tutorial_server_variable.c)
target_link_libraries(tutorial_server_variable open62541 ${open62541_LIBRARIES})

add_executable(tutorial_server_datasource tutorial_server_datasource.c)
target_link_libraries(tutorial_server_datasource open62541 ${open62541_LIBRARIES})

add_executable(tutorial_server_variabletype tutorial_server_variabletype.c)
target_link_libraries(tutorial_server_variabletype open62541 ${open62541_LIBRARIES})

add_executable(tutorial_server_object tutorial_server_object.c)
target_link_libraries(tutorial_server_object open62541 ${open62541_LIBRARIES})

if(UA_ENABLE_METHODCALLS)
  add_executable(tutorial_server_method tutorial_server_method.c)
  target_link_libraries(tutorial_server_method open62541 ${open62541_LIBRARIES})
endif()

add_executable(tutorial_client_firststeps tutorial_client_firststeps.c)
target_link_libraries(tutorial_client_firststeps open62541 ${open62541_LIBRARIES})

##################
# Example Server #
##################

add_executable(server server.c)
target_link_libraries(server open62541 ${open62541_LIBRARIES})

if(UA_ENABLE_NONSTANDARD_UDP)
  add_executable(server_udp server_udp.c ${PROJECT_SOURCE_DIR}/plugins/ua_network_udp.c)
  target_link_libraries(server_udp open62541 ${open62541_LIBRARIES})
endif()

##################
# Example Client #
##################

add_executable(client client.c)
target_link_libraries(client open62541 ${open62541_LIBRARIES})

####################
# Feature Examples #
####################

add_executable(server_mainloop server_mainloop.c)
target_link_libraries(server_mainloop open62541 ${open62541_LIBRARIES})

add_executable(server_instantiation server_instantiation.c)
target_link_libraries(server_instantiation open62541 ${open62541_LIBRARIES})

add_executable(server_repeated_job server_repeated_job.c)
target_link_libraries(server_repeated_job open62541 ${open62541_LIBRARIES})

add_executable(server_inheritance server_inheritance.c)
target_link_libraries(server_inheritance open62541 ${open62541_LIBRARIES})

if(NOT BUILD_SHARED_open62541_LIBRARIES AND UA_BUILD_EXAMPLES_NODESET_COMPILER)
  # example information model from nodeset xml
  add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/nodeset.h ${PROJECT_BINARY_DIR}/src_generated/nodeset.c
                    PRE_BUILD
                    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
                                                 -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist_FullNS0.txt
                                                 ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/Opc.Ua.NodeSet2.xml
                                                 ${PROJECT_SOURCE_DIR}/examples/server_nodeset.xml
                                                 ${PROJECT_BINARY_DIR}/src_generated/nodeset
                    DEPENDS ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
                            ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/open62541_MacroHelper.py
                            ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_builtin_types.py
                            ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_constants.py
                            ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_namespace.py
                            ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_node_types.py
                            ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist_FullNS0.txt
                            ${PROJECT_SOURCE_DIR}/examples/server_nodeset.xml)
						  
  # needs internal methods which are not exported in the dynamic lib
  add_executable(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/nodeset.c)
  target_link_libraries(server_nodeset open62541 ${open62541_LIBRARIES})
  target_include_directories(server_nodeset PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
endif()

if(UA_BUILD_SELFSIGNED_CERTIFICATE)
  find_package(OpenSSL REQUIRED)
  add_custom_command(OUTPUT server_cert.der ca.crt
                     COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${CMAKE_CURRENT_BINARY_DIR}
                     DEPENDS ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py
                             ${PROJECT_SOURCE_DIR}/tools/certs/localhost.cnf)
                             
  add_custom_target(selfsigned ALL DEPENDS server_cert.der ca.crt)

  add_executable(server_certificate server_certificate.c server_cert.der ca.crt)
  target_link_libraries(server_certificate open62541 ${open62541_LIBRARIES})
endif()
