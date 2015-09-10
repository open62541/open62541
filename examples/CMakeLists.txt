include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/src_generated)

set(LIBS open62541-static)
if(NOT WIN32)
    list(APPEND LIBS pthread)
    if (NOT APPLE)
        list(APPEND LIBS rt)
    endif()
else()
    list(APPEND LIBS ws2_32)
endif()
if(ENABLE_MULTITHREADING)
    list(APPEND LIBS urcu-cds urcu urcu-common)
endif(ENABLE_MULTITHREADING)

add_executable(server_readspeed server_readspeed.c)
target_link_libraries(server_readspeed ${LIBS})

add_executable(server_variable server_variable.c)
target_link_libraries(server_variable ${LIBS})

add_executable(server_datasource server_datasource.c)
target_link_libraries(server_datasource ${LIBS})

add_executable(server_firstSteps server_firstSteps.c)
target_link_libraries(server_firstSteps ${LIBS})

add_executable(client_firstSteps client_firstSteps.c)
target_link_libraries(client_firstSteps ${LIBS})

if(NOT ENABLE_AMALGAMATION)
	add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src_generated/nodeset.h ${PROJECT_BINARY_DIR}/src_generated/nodeset.c
		           PRE_BUILD
		           COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py -i ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist_FullNS0.txt ${PROJECT_SOURCE_DIR}/tools/schema/namespace0/Opc.Ua.NodeSet2.xml ${PROJECT_SOURCE_DIR}/examples/server_nodeset.xml ${PROJECT_BINARY_DIR}/src_generated/nodeset
		           DEPENDS ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/generate_open62541CCode.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/logger.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/open62541_MacroHelper.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_builtin_types.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_constants.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_namespace.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/ua_node_types.py
		                   ${PROJECT_SOURCE_DIR}/tools/pyUANamespace/NodeID_Blacklist_FullNS0.txt
		                   ${PROJECT_SOURCE_DIR}/examples/server_nodeset.xml)
		           
	add_executable(server_nodeset server_nodeset.c ${PROJECT_BINARY_DIR}/src_generated/nodeset.c)
	target_link_libraries(server_nodeset ${LIBS})
endif()

if(ENABLE_METHODCALLS)
  add_executable(server_method server_method.c)
  target_link_libraries(server_method ${LIBS})
endif()
