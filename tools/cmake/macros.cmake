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
endfunction()
