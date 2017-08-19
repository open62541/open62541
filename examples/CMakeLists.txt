include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR})

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
  add_executable(${EXAMPLE_NAME} ${STATIC_OBJECTS} ${EXAMPLE_SOURCE} ${ARGN})
  target_link_libraries(${EXAMPLE_NAME} open62541 ${open62541_LIBRARIES})
  assign_source_group(${EXAMPLE_SOURCE})
  add_dependencies(${EXAMPLE_NAME} open62541-amalgamation-header open62541-amalgamation-source)
  set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER "open62541/examples")
  set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples")
  if(UA_COMPILE_AS_CXX)
    set_source_files_properties(${EXAMPLE_SOURCE} PROPERTIES LANGUAGE CXX)
  endif()
endmacro()

#############
# Tutorials #
#############

add_example(tutorial_datatypes tutorial_datatypes.c)

add_example(tutorial_server_firststeps tutorial_server_firststeps.c)

add_example(tutorial_server_variable tutorial_server_variable.c)

add_example(tutorial_server_datasource tutorial_server_datasource.c)

add_example(tutorial_server_variabletype tutorial_server_variabletype.c)

add_example(tutorial_server_object tutorial_server_object.c)

if(UA_ENABLE_METHODCALLS)
  add_example(tutorial_server_method tutorial_server_method.c)
endif()

add_example(tutorial_client_firststeps tutorial_client_firststeps.c)

##################
# Example Server #
##################

add_example(server server.c)

##################
# Example Client #
##################

add_example(client client.c)

####################
# Feature Examples #
####################

add_example(server_mainloop server_mainloop.c)

add_example(server_instantiation server_instantiation.c)

add_example(server_repeated_job server_repeated_job.c)

add_example(server_inheritance server_inheritance.c)

if(UA_BUILD_EXAMPLES_NODESET_COMPILER)
  if(BUILD_SHARED_LIBS)
    message(FATAL_ERROR "The nodeset compiler currently requires static linking to access internal API")
  endif()

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
  add_example(server_nodeset ${PROJECT_BINARY_DIR}/src_generated/nodeset.c server_nodeset.c)
  target_include_directories(server_nodeset PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
  set_target_properties(server_nodeset PROPERTIES COMPILE_FLAGS "-Wno-pedantic -Wno-sign-conversion")
endif()

if(UA_BUILD_SELFSIGNED_CERTIFICATE)
  find_package(OpenSSL REQUIRED)
  add_custom_command(OUTPUT server_cert.der ca.crt
                     COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py ${CMAKE_CURRENT_BINARY_DIR}
                     DEPENDS ${PROJECT_SOURCE_DIR}/tools/certs/create_self-signed.py
                             ${PROJECT_SOURCE_DIR}/tools/certs/localhost.cnf)
  add_custom_target(selfsigned ALL DEPENDS server_cert.der ca.crt)
  add_executable(server_certificate server_certificate.c ${STATIC_OBJECTS} server_cert.der ca.crt)
  target_link_libraries(server_certificate open62541 ${open62541_LIBRARIES})
endif()

if(UA_ENABLE_DISCOVERY)
    add_example(discovery_server_lds discovery/server_lds.c)

    add_example(discovery_server_register discovery/server_register.c)

    add_example(discovery_client_find_servers discovery/client_find_servers.c)
    if(UA_ENABLE_DISCOVERY_MULTICAST)
        add_example(discovery_server_multicast discovery/server_multicast.c)
    endif()
endif()
