include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugins)
include_directories(${PROJECT_BINARY_DIR})

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/examples)

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

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
  add_executable(${EXAMPLE_NAME} $<TARGET_OBJECTS:open62541-object> ${EXAMPLE_SOURCE})
  target_link_libraries(${EXAMPLE_NAME} ${LIBS})
  assign_source_group(${EXAMPLE_SOURCE})
  add_dependencies(${EXAMPLE_NAME} open625451_amalgamation)
  set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER "open62541/examples")
  if(UA_COMPILE_AS_CXX)
    set_source_files_properties(${EXAMPLE_SOURCE} PROPERTIES LANGUAGE CXX)
  endif()
endmacro()

##################
# Example Server #
##################

add_example(server server.c)

add_example(server_firststeps tutorials/server_firststeps.c)

##################
# Example Client #
##################

add_example(client client.c)

add_example(client_firststeps tutorials/client_firststeps.c)

####################
# Feature Examples #
####################

add_example(server_variable server_variable.c)

add_example(server_mainloop server_mainloop.c)

add_example(server_datasource server_datasource.c)

add_example(server_instantiation server_instantiation.c)

add_example(server_repeated_job server_repeated_job.c)

add_example(server_readspeed server_readspeed.c)
target_include_directories(server_readspeed PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header

add_example(server_inheritance server_inheritance.c)

if(UA_BUILD_EXAMPLES_NODESET_COMPILER)
  add_executable(server_nodeset "server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/nodeset.c")
  target_include_directories(server_nodeset PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/deps) # needs an internal header
endif()

if(UA_ENABLE_METHODCALLS)
  add_example(server_method server_method.c)
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

if(UA_ENABLE_DISCOVERY)
    add_example(discovery_server_discovery discovery/server_discovery.c)

    add_example(discovery_server_register discovery/server_register.c)

    add_example(discovery_client_find_servers discovery/client_find_servers.c)
endif()
