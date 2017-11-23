include_directories(${PROJECT_SOURCE_DIR}/src)

#############################
# Compiled binaries folders #
#############################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/corpus_generator)

add_definitions(-DUA_CORPUS_OUTPUT_DIR="${PROJECT_BINARY_DIR}/corpus")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/corpus)

add_executable(corpus_generator corpus_generator.c)
target_link_libraries(corpus_generator open62541 ${open62541_LIBRARIES})
assign_source_group(corpus_generator)
add_dependencies(corpus_generator open62541-amalgamation-header)
set_target_properties(corpus_generator PROPERTIES FOLDER "open62541/corpus_generator")
set_target_properties(corpus_generator PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/corpus_generator")