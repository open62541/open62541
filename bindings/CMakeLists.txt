FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(UseSWIG)

if(ENABLE_BINDING_LUA)
    FIND_PACKAGE(Lua51)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lua)
    add_custom_command (OUTPUT ${PROJECT_BINARY_DIR}/lua/lua_wrap.c 
                        COMMAND ${SWIG_EXECUTABLE} -lua
                        -I${PROJECT_SOURCE_DIR}/include
                        -o ${PROJECT_BINARY_DIR}/lua/lua_wrap.c
                        ${PROJECT_SOURCE_DIR}/bindings/open62541.i
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/open62541.i)
    add_library(open62541-lua SHARED ${PROJECT_BINARY_DIR}/lua/lua_wrap.c)
    set_target_properties(open62541-lua PROPERTIES COMPILE_FLAGS "-Wno-error")
    target_include_directories(open62541-lua PUBLIC ${LUA_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/include
                                                    ${PROJECT_BINARY_DIR}/src_generated)
    target_link_libraries(open62541-lua open62541-static)
    set_target_properties(open62541-lua PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lua
                                                   OUTPUT_NAME open62541 PREFIX "")
endif()

if(ENABLE_BINDING_PYTHON)
    FIND_PACKAGE(PythonLibs)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/python)
    add_custom_command (OUTPUT ${PROJECT_BINARY_DIR}/python/python_wrap.c 
                        COMMAND ${SWIG_EXECUTABLE} -python
                        -I${PROJECT_SOURCE_DIR}/include
                        -o ${PROJECT_BINARY_DIR}/python/python_wrap.c
                        ${PROJECT_SOURCE_DIR}/bindings/open62541.i
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/open62541.i)
    add_library(open62541-python SHARED ${PROJECT_BINARY_DIR}/python/python_wrap.c)
    set_target_properties(open62541-python PROPERTIES COMPILE_FLAGS "-Wno-error")
    target_include_directories(open62541-python PUBLIC ${PYTHON_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/include
                                                       ${PROJECT_BINARY_DIR}/src_generated)
    target_link_libraries(open62541-python open62541-static)
    set_target_properties(open62541-python PROPERTIES
                                           LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/python
                                           OUTPUT_NAME _open62541 PREFIX "")
endif()
