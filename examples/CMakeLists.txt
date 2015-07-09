include_directories(${PROJECT_SOURCE_DIR}/include)

set(LIBS open62541)
if(NOT WIN32)
    list(APPEND LIBS rt pthread)
else()
    list(APPEND LIBS ws2_32)
endif()
if(MULTITHREADING)
    list(APPEND LIBS urcu-cds urcu urcu-common)
endif(MULTITHREADING)

add_executable(server_variable server_variable.c)
target_link_libraries(server_variable ${LIBS})

add_executable(server_repeated_job server_repeated_job.c)
target_link_libraries(server_repeated_job ${LIBS})

if(ENABLE_METHODCALLS)
  add_executable(server_method server_method.c)
  target_link_libraries(server_method ${LIBS})
endif()