#Add a new architecture to to the lists of available architectures
FUNCTION(ua_add_architecture)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURES ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_add_architecture)

#Include folders to the compilation
FUNCTION(ua_include_directories)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_INCLUDE_DIRECTORIES ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_include_directories)

#Add a new header file to the architecture group
FUNCTION(ua_add_architecture_header)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURE_HEADERS ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_add_architecture_header)

#Add a new header file to the architecture group at the beginning of it
FUNCTION(ua_add_architecture_header_beginning)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURE_HEADERS_BEGINNING ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_add_architecture_header_beginning)

#Add a new source file to the architecture group
FUNCTION(ua_add_architecture_file)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURE_SOURCES ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_add_architecture_file)

#Add definitions to the compilations that are exclusive for the selected architecture
FUNCTION(ua_architecture_add_definitions)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURE_ADD_DEFINITIONS ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_architecture_add_definitions)

#Remove definitions from the compilations that are exclusive for the selected architecture
FUNCTION(ua_architecture_remove_definitions)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURE_REMOVE_DEFINITIONS ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_architecture_remove_definitions)

#Add libraries to be linked to the comnpilation that are exclusive for the selected architecture
FUNCTION(ua_architecture_append_to_library)
    FOREACH(ARG ${ARGV})
        set_property(GLOBAL APPEND PROPERTY UA_ARCHITECTURE_APPEND_TO_LIBRARY ${ARG})
    ENDFOREACH(ARG)
ENDFUNCTION(ua_architecture_append_to_library)


# --------------- Generate Datatypes ---------------------
#
# Generates Datatype definition based on the .csv and .bsd files of a nodeset.
# The result of the generation will be C Code which can be compiled with the rest of the stack.
# Some nodesets come with custom datatypes. These datatype structures first need to be
# generated so that the nodeset can use these types.
#
# The resulting files will be put into OUTPUT_DIR with the names:
# - NAME_generated.c
# - NAME_generated.h
# - NAME_generated_encoding_binary.h
# - NAME_generated_handling.h
#
# The cmake resulting cmake target will be named like this:
#   open62541-generator-${TARGET_SUFFIX}
#
# The following arguments are accepted:
#   Options:
#
#   [BUILTIN]       Optional argument. If given, then builtin types will be generated.
#
#   Arguments taking one value:
#
#   NAME            Full name of the generated files, e.g. ua_types_di
#   TARGET_SUFFIX   Suffix for the resulting target. e.g. types-di
#   NAMESPACE_IDX   Namespace index of the nodeset, when it is loaded into the server. This index
#                   is used for the node ids withing the types array and is currently not determined automatically.
#                   Make sure that it matches the namespace index in the server.
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is '${PROJECT_BINARY_DIR}/src_generated'
#   FILE_CSV        Path to the .csv file containing the node ids, e.g. 'OpcUaDiModel.csv'
#
#   Arguments taking multiple values:
#
#   FILES_BSD        Path to the .bsd file containing the type definitions, e.g. 'Opc.Ua.Di.Types.bsd'. Multiple files can be
#                   passed which will all combined to one resulting code.
#   [FILES_SELECTED] Optional path to a simple text file which contains a list of types which should be included in the generation.
#                   The file should contain one type per line. Multiple files can be passed to this argument.
#
#
function(ua_generate_datatypes)
    set(options BUILTIN)
    set(oneValueArgs NAME TARGET_SUFFIX NAMESPACE_IDX OUTPUT_DIR FILE_CSV)
    set(multiValueArgs FILES_BSD FILES_SELECTED)
    cmake_parse_arguments(PARSE_ARGV 0 UA_GEN_DT "${options}" "${oneValueArgs}"
                          "${multiValueArgs}")


    # ------ Argument checking -----
    if(NOT DEFINED UA_GEN_DT_NAMESPACE_IDX AND NOT "${UA_GEN_DT_NAMESPACE_IDX}" STREQUAL "0")
        message(FATAL_ERROR "ua_generate_datatype function requires a value for the NAMESPACE_IDX argument")
    endif()
    if(NOT UA_GEN_DT_NAME OR "${UA_GEN_DT_NAME}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_datatype function requires a value for the NAME argument")
    endif()
    if(NOT UA_GEN_DT_TARGET_SUFFIX OR "${UA_GEN_DT_TARGET_SUFFIX}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_datatype function requires a value for the TARGET_SUFFIX argument")
    endif()
    if(NOT UA_GEN_DT_FILE_CSV OR "${UA_GEN_DT_FILE_CSV}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_datatype function requires a value for the FILE_CSV argument")
    endif()
    if(NOT UA_GEN_DT_FILES_BSD OR "${UA_GEN_DT_FILES_BSD}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_datatype function requires a value for the FILES_BSD argument")
    endif()

    # Set default value for output dir
    if(NOT UA_GEN_DT_OUTPUT_DIR)
        set(UA_GEN_DT_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated)
    endif()

    # ------ Add custom command and target -----

    set(UA_GEN_DT_NO_BUILTIN "--no-builtin")
    if (UA_GEN_DT_BUILTIN)
        set(UA_GEN_DT_NO_BUILTIN "")
    endif()


    set(SELECTED_TYPES_TMP "")
    foreach(f ${UA_GEN_DT_FILES_SELECTED})
        set(SELECTED_TYPES_TMP ${SELECTED_TYPES_TMP} "--selected-types=${f}")
    endforeach()

    set(BSD_FILES_TMP "")
    foreach(f ${UA_GEN_DT_FILES_BSD})
        set(BSD_FILES_TMP ${BSD_FILES_TMP} "--type-bsd=${f}")
    endforeach()

    add_custom_command(OUTPUT ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
                       ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h
                       ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_handling.h
                       ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_encoding_binary.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       --namespace=${UA_GEN_DT_NAMESPACE_IDX}
                       ${SELECTED_TYPES_TMP}
                       ${BSD_FILES_TMP}
                       --type-csv=${UA_GEN_DT_FILE_CSV}
                       ${UA_GEN_DT_NO_BUILTIN}
                       ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}
                       DEPENDS ${PROJECT_SOURCE_DIR}/tools/generate_datatypes.py
                       ${UA_GEN_DT_FILES_BSD}
                       ${UA_GEN_DT_FILE_CSV}
                       ${UA_GEN_DT_FILES_SELECTED})
    add_custom_target(open62541-generator-${UA_GEN_DT_TARGET_SUFFIX} DEPENDS
                      ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
                      ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h
                      ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_handling.h
                      ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_encoding_binary.h
                      )

    if(UA_COMPILE_AS_CXX)
        set_source_files_properties(${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c PROPERTIES LANGUAGE CXX)
    endif()
endfunction()


# --------------- Generate Nodeset ---------------------
#
# Generates C code for the given NodeSet2.xml file.
# This C code can be used to initialize the server.
#
# The resulting files will be put into OUTPUT_DIR with the names:
# - ua_namespace_NAME.c
# - ua_namespace_NAME.h
#
# The resulting cmake target will be named like this:
#   open62541-generator-ns-${NAME}
#
# The following arguments are accepted:
#   Options:
#
#   [INTERNAL]      Optional argument. If given, then the generated node set code will use internal headers.
#
#   Arguments taking one value:
#
#   NAME            Full name of the generated files, e.g. ua_types_di
#   [TYPES_ARRAY]   Optional name of the types array containing the custom datatypes of this node set.
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is '${PROJECT_BINARY_DIR}/src_generated'
#   [ENCODE_BINARY_SIZE]    Optional array size for binary encoding extension objects.
#   [IGNORE]        Optional file containing a list of node ids which should be ignored. The file should have one id per line.
#
#   Arguments taking multiple values:
#
#   FILE            Path to the NodeSet2.xml file. Multiple values can be passed. These nodesets will be combined into one output.
#   [DEPENDS_TYPES]   Optional list of types array which match with the DEPENDS_NS node sets. e.g. 'UA_TYPES;UA_TYPES_DI'
#   [DEPENDS_NS]      Optional list of NodeSet2.xml files which are a dependency of this node set.
#   [DEPENDS_TARGET]  Optional list of CMake targets this nodeset depends on.
#
#
function(ua_generate_nodeset)

    set(options INTERNAL )
    set(oneValueArgs NAME TYPES_ARRAY OUTPUT_DIR ENCODE_BINARY_SIZE IGNORE)
    set(multiValueArgs FILE DEPENDS_TYPES DEPENDS_NS DEPENDS_TARGET)
    cmake_parse_arguments(PARSE_ARGV 0 UA_GEN_NS "${options}" "${oneValueArgs}"
                          "${multiValueArgs}")



    # ------ Argument checking -----
    if(NOT UA_GEN_NS_NAME OR "${UA_GEN_NS_NAME}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeset function requires a value for the NAME argument")
    endif()

    if(NOT UA_GEN_NS_FILE OR "${UA_GEN_NS_FILE}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeset function requires a value for the FILE argument")
    endif()

    # Set default value for output dir
    if(NOT UA_GEN_NS_OUTPUT_DIR)
        set(UA_GEN_NS_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated)
    endif()

    list(LENGTH UA_GEN_NS_DEPENDS_TYPES DEPENDS_TYPES_LEN)
    list(LENGTH UA_GEN_NS_DEPENDS_NS DEPENDS_NS_LEN)

    if(NOT DEPENDS_TYPES_LEN EQUAL DEPENDS_NS_LEN)
        message(FATAL_ERROR "ua_generate_nodeset parameters DEPENDS_NS and DEPENDS_TYPES must have the same number of list elements")
    endif()

    # ------ Add custom command and target -----

    set(GEN_INTERNAL_HEADERS "")
    if (UA_GEN_NS_INTERNAL)
        set(GEN_INTERNAL_HEADERS "--internal-headers")
    endif()

    set(GEN_NS0 "")
    set(TARGET_SUFFIX "ns-${UA_GEN_NS_NAME}")
    set(FILE_SUFFIX "_${UA_GEN_NS_NAME}")

    if ("${UA_GEN_NS_NAME}" STREQUAL "ns0")
        set(GEN_NS0 "--generate-ns0")
        set(TARGET_SUFFIX "namespace")
        set(FILE_SUFFIX "0")
    endif()

    set(GEN_IGNORE "")
    if (UA_GEN_NS_IGNORE)
        set(GEN_IGNORE "--ignore=${UA_GEN_NS_IGNORE}")
    endif()

    set(GEN_BIN_SIZE "")
    if (UA_GEN_NS_ENCODE_BINARY_SIZE OR "${UA_GEN_NS_ENCODE_BINARY_SIZE}" STREQUAL "0")
        set(GEN_BIN_SIZE "--encode-binary-size=${UA_GEN_NS_ENCODE_BINARY_SIZE}")
    endif()


    set(TYPES_ARRAY_LIST "")
    foreach(f ${UA_GEN_NS_DEPENDS_TYPES})
        set(TYPES_ARRAY_LIST ${TYPES_ARRAY_LIST} "--types-array=${f}")
    endforeach()
    if(UA_GEN_NS_TYPES_ARRAY)
        set(TYPES_ARRAY_LIST ${TYPES_ARRAY_LIST} "--types-array=${UA_GEN_NS_TYPES_ARRAY}")
    endif()

    set(DEPENDS_FILE_LIST "")
    foreach(f ${UA_GEN_NS_DEPENDS_NS})
        set(DEPENDS_FILE_LIST ${DEPENDS_FILE_LIST} "--existing=${f}")
    endforeach()
    set(FILE_LIST "")
    foreach(f ${UA_GEN_NS_FILE})
        set(FILE_LIST ${FILE_LIST} "--xml=${f}")
    endforeach()

    add_custom_command(OUTPUT ${UA_GEN_NS_OUTPUT_DIR}/ua_namespace${FILE_SUFFIX}.c
                       ${UA_GEN_NS_OUTPUT_DIR}/ua_namespace${FILE_SUFFIX}.h
                       PRE_BUILD
                       COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       ${GEN_INTERNAL_HEADERS}
                       ${GEN_NS0}
                       ${GEN_BIN_SIZE}
                       ${GEN_IGNORE}
                       ${TYPES_ARRAY_LIST}
                       ${DEPENDS_FILE_LIST}
                       ${FILE_LIST}
                       ${PROJECT_BINARY_DIR}/src_generated/ua_namespace${FILE_SUFFIX}
                       DEPENDS
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset_compiler.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/nodeset.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/datatypes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_nodes.py
                       ${PROJECT_SOURCE_DIR}/tools/nodeset_compiler/backend_open62541_datatypes.py
                       ${UA_GEN_NS_FILE}
                       ${UA_GEN_NS_DEPENDS_NS}
                       )

    add_custom_target(open62541-generator-${TARGET_SUFFIX}
                      DEPENDS
                      ${PROJECT_BINARY_DIR}/src_generated/ua_namespace${FILE_SUFFIX}.c
                      ${PROJECT_BINARY_DIR}/src_generated/ua_namespace${FILE_SUFFIX}.h)
    if (UA_GEN_NS_DEPENDS_TARGET)
        add_dependencies(open62541-generator-${TARGET_SUFFIX} ${UA_GEN_NS_DEPENDS_TARGET})
    endif()

    if(UA_COMPILE_AS_CXX)
        set_source_files_properties(${UA_GEN_NS_OUTPUT_DIR}/ua_namespace${FILE_SUFFIX}.c PROPERTIES LANGUAGE CXX)
    endif()
endfunction()
