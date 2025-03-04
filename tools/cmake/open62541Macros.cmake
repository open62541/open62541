if(NOT open62541_TOOLS_DIR OR "${open62541_TOOLS_DIR}" STREQUAL "")
    set(open62541_TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
endif()

# --------------- Generate NodeIDs header ---------------------
#
# Generates header file from .csv which contains defines for every
# node id to be used instead of numeric node ids.
#
# The resulting files will be put into OUTPUT_DIR with the names:
# - NAME.h
#
# The following arguments are accepted:
#   Options:
#
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically attached to the server.
#
#   Arguments taking one value:
#
#   NAME            Full name of the generated files, e.g. di_nodeids
#   TARGET_SUFFIX   Suffix for the resulting target. e.g. ids-di
#   [TARGET_PREFIX] Optional prefix for the resulting target. Default `open62541-generator`
#   ID_PREFIX       Prefix for the generated node ID defines, e.g. NS_DI
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is '${PROJECT_BINARY_DIR}/src_generated'
#   FILE_CSV        Path to the .csv file containing the node ids, e.g. 'OpcUaDiModel.csv'
#
function(ua_generate_nodeid_header)
    find_package(Python3 REQUIRED)

    set(options AUTOLOAD)
    set(oneValueArgs NAME ID_PREFIX OUTPUT_DIR FILE_CSV TARGET_SUFFIX TARGET_PREFIX)
    set(multiValueArgs )
    cmake_parse_arguments(UA_GEN_ID "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(NOT UA_GEN_ID_TARGET_SUFFIX OR "${UA_GEN_ID_TARGET_SUFFIX}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeid_header function requires a value for the TARGET_SUFFIX argument")
    endif()

    # Set default value for output dir
    if(NOT UA_GEN_ID_OUTPUT_DIR OR "${UA_GEN_ID_OUTPUT_DIR}" STREQUAL "")
        set(UA_GEN_ID_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    endif()
    # Set default target prefix
    if(NOT UA_GEN_ID_TARGET_PREFIX OR "${UA_GEN_ID_TARGET_PREFIX}" STREQUAL "")
        set(UA_GEN_ID_TARGET_PREFIX "open62541-generator")
    endif()

    # Replace dash with underscore to make valid c literal
    string(REPLACE "-" "_" UA_GEN_ID_NAME ${UA_GEN_ID_NAME})

    if(NOT TARGET ${UA_GEN_ID_TARGET_PREFIX}-${UA_GEN_ID_TARGET_SUFFIX})
        add_custom_target(${UA_GEN_ID_TARGET_PREFIX}-${UA_GEN_ID_TARGET_SUFFIX} DEPENDS
            ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}.h
        )
    endif()

    if(UA_GEN_ID_AUTOLOAD AND UA_ENABLE_NODESET_INJECTOR)
        list(APPEND UA_NODESETINJECTOR_GENERATORS ${UA_GEN_ID_TARGET_PREFIX}-${UA_GEN_ID_TARGET_SUFFIX})
        set(UA_NODESETINJECTOR_GENERATORS ${UA_NODESETINJECTOR_GENERATORS} PARENT_SCOPE)
    endif()

    # Make sure that the output directory exists
    if(NOT EXISTS ${UA_GEN_ID_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${UA_GEN_ID_OUTPUT_DIR})
    endif()

    # Header containing defines for all NodeIds
    add_custom_command(OUTPUT ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}.h
        COMMAND ${Python3_EXECUTABLE} ${open62541_TOOLS_DIR}/generate_nodeid_header.py
        ${UA_GEN_ID_FILE_CSV}  ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME} ${UA_GEN_ID_ID_PREFIX}
        DEPENDS ${open62541_TOOLS_DIR}/generate_nodeid_header.py
        ${UA_GEN_ID_FILE_CSV})
endfunction()


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
# - NAME_generated_handling.h
# - NAME_generated.rst (optional)
#
# The cmake resulting cmake target will be named like this:
#   open62541-generator-${TARGET_SUFFIX}
#
# The following arguments are accepted:
#   Options:
#
#   [BUILTIN]       Optional argument. If given, then builtin types will be generated.
#   [INTERNAL]      Optional argument. If given, then the given types file is seen as internal file (e.g. does not require a .csv)
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically attached to the server.
#   [GEN_DOC]       Optional argument. If given, a .rst file for documenting the generated datatypes is generated.
#
#   Arguments taking one value:
#
#   NAME            Full name of the generated files, e.g. ua_types_di
#   TARGET_SUFFIX   Suffix for the resulting target. e.g. types-di
#   [TARGET_PREFIX] Optional prefix for the resulting target. Default `open62541-generator`
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is '${PROJECT_BINARY_DIR}/src_generated'
#   FILE_CSV        Path to the .csv file containing the node ids, e.g. 'OpcUaDiModel.csv'
#
#   Arguments taking multiple values:
#
#   FILES_BSD        Path to the .bsd file containing the type definitions, e.g. 'Opc.Ua.Di.Types.bsd'. Multiple files can be
#                   passed which will all combined to one resulting code.
#   IMPORT_BSD      Combination of types array and path to the .bsd file containing additional type definitions referenced by
#                   the FILES_BSD files. The value is separated with a hash sign, i.e.
#                   'UA_TYPES#${UA_SCHEMA_DIR}/Opc.Ua.Types.bsd'
#                   Multiple files can be passed which will all be imported.
#   [FILES_SELECTED] Optional path to a simple text file which contains a list of types which should be included in the generation.
#                   The file should contain one type per line. Multiple files can be passed to this argument.
#   NAMESPACE_MAP   [Deprecated]Array of Namespace index mappings to indicate the final namespace index of a namespace uri when the server is started.
#                   This is required to correctly map datatype node ids to the resulting server namespace index.
#                   "0:http://opcfoundation.org/UA/" is added by default.
#                   Example: ["2:http://example.org/UA/"]
#
#
function(ua_generate_datatypes)
    find_package(Python3 REQUIRED)

    set(options BUILTIN INTERNAL AUTOLOAD GEN_DOC)
    set(oneValueArgs NAME TARGET_SUFFIX TARGET_PREFIX OUTPUT_DIR FILE_XML FILE_CSV)
    set(multiValueArgs FILES_BSD IMPORT_BSD FILES_SELECTED NAMESPACE_MAP)
    cmake_parse_arguments(UA_GEN_DT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(NOT DEFINED open62541_TOOLS_DIR)
        message(FATAL_ERROR "open62541_TOOLS_DIR must point to the open62541 tools directory")
    endif()

    # ------ Argument checking -----
    if(DEFINED UA_GEN_DT_NAMESPACE_MAP)
        message(WARNING "NAMESPACE_MAP argument is deprecated and no longer has any effect. The index of the nodeset is set automatically.")
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
    if(NOT UA_GEN_DT_OUTPUT_DIR OR "${UA_GEN_DT_OUTPUT_DIR}" STREQUAL "")
        set(UA_GEN_DT_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    endif()
    # Set default target prefix
    if(NOT UA_GEN_DT_TARGET_PREFIX OR "${UA_GEN_DT_TARGET_PREFIX}" STREQUAL "")
        set(UA_GEN_DT_TARGET_PREFIX "open62541-generator")
    endif()

    # ------ Add custom command and target -----

    set(NAMESPACE_MAP_TMP "")
    foreach(f ${UA_GEN_DT_NAMESPACE_MAP})
        set(NAMESPACE_MAP_TMP ${NAMESPACE_MAP_TMP} "--namespaceMap=${f}")
    endforeach()

    set(UA_GEN_DT_NO_BUILTIN "--no-builtin")
    if (UA_GEN_DT_BUILTIN)
        set(UA_GEN_DT_NO_BUILTIN "")
    endif()

    set(UA_GEN_DOC_ARG "")
    if(UA_GEN_DT_GEN_DOC)
        set(UA_GEN_DOC_ARG "--gen-doc")
    endif()

    set(UA_GEN_DT_INTERNAL_ARG "")
    if (UA_GEN_DT_INTERNAL)
        set(UA_GEN_DT_INTERNAL_ARG "--internal")
    endif()

    set(SELECTED_TYPES_TMP "")
    foreach(f ${UA_GEN_DT_FILES_SELECTED})
        set(SELECTED_TYPES_TMP ${SELECTED_TYPES_TMP} "--selected-types=${f}")
    endforeach()

    set(BSD_FILES_TMP "")
    foreach(f ${UA_GEN_DT_FILES_BSD})
        set(BSD_FILES_TMP ${BSD_FILES_TMP} "--type-bsd=${f}")
    endforeach()

    set(IMPORT_BSD_TMP "")
    foreach(f ${UA_GEN_DT_IMPORT_BSD})
        set(IMPORT_BSD_TMP ${IMPORT_BSD_TMP} "--import=${f}")
    endforeach()

    # Make sure that the output directory exists
    if(NOT EXISTS ${UA_GEN_DT_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${UA_GEN_DT_OUTPUT_DIR})
    endif()

    # Replace dash with underscore to make valid c literal
    string(REPLACE "-" "_" UA_GEN_DT_NAME ${UA_GEN_DT_NAME})

    if((MINGW) AND (DEFINED ENV{SHELL}))
        # fix issue 4156 that MINGW will do automatic Windows Path Conversion
        # powershell handles Windows Path correctly
        # MINGW SHELL only accept environment variable with "env"
        set(ARG_CONV_EXCL_ENV env MSYS2_ARG_CONV_EXCL=--import)
    endif()

    set(FILE_XML "")
    if (UA_GEN_DT_FILE_XML)
        set(FILE_XML "--xml=${UA_GEN_DT_FILE_XML}")
    endif()

    add_custom_command(OUTPUT ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
        ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h
        ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_handling.h
        COMMAND ${ARG_CONV_EXCL_ENV} ${Python3_EXECUTABLE} ${open62541_TOOLS_DIR}/generate_datatypes.py
        ${NAMESPACE_MAP_TMP}
        ${SELECTED_TYPES_TMP}
        ${BSD_FILES_TMP}
        ${IMPORT_BSD_TMP}
        ${FILE_XML}
        --type-csv=${UA_GEN_DT_FILE_CSV}
        ${UA_GEN_DT_NO_BUILTIN}
        ${UA_GEN_DT_INTERNAL_ARG}
        ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}
        ${UA_GEN_DOC_ARG}
        DEPENDS ${open62541_TOOLS_DIR}/generate_datatypes.py
                ${open62541_TOOLS_DIR}/nodeset_compiler/backend_open62541_typedefinitions.py
        ${UA_GEN_DT_FILES_BSD}
        ${UA_GEN_DT_FILE_XML}
        ${UA_GEN_DT_FILE_CSV}
        ${UA_GEN_DT_FILES_SELECTED})
    if(NOT TARGET ${UA_GEN_DT_TARGET_PREFIX}-${UA_GEN_DT_TARGET_SUFFIX})
        add_custom_target(${UA_GEN_DT_TARGET_PREFIX}-${UA_GEN_DT_TARGET_SUFFIX} DEPENDS
                          ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
                          ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h
                          ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_handling.h)
    endif()

    if(UA_GEN_DT_AUTOLOAD AND UA_ENABLE_NODESET_INJECTOR)
        list(APPEND UA_NODESETINJECTOR_GENERATORS ${UA_GEN_DT_TARGET_PREFIX}-${UA_GEN_DT_TARGET_SUFFIX})
        set(UA_NODESETINJECTOR_GENERATORS ${UA_NODESETINJECTOR_GENERATORS} PARENT_SCOPE)
        list(APPEND UA_NODESETINJECTOR_SOURCE_FILES  ${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_GEN_DT_NAME}_generated.c)
        set(UA_NODESETINJECTOR_SOURCE_FILES ${UA_NODESETINJECTOR_SOURCE_FILES} PARENT_SCOPE)
        list(APPEND UA_NODESETINJECTOR_HEADER_FILES  ${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_GEN_DT_NAME}_generated.h)
        set(UA_NODESETINJECTOR_HEADER_FILES ${UA_NODESETINJECTOR_HEADER_FILES} PARENT_SCOPE)
    endif()

    string(TOUPPER "${UA_GEN_DT_NAME}" GEN_NAME_UPPER)
    set(UA_${GEN_NAME_UPPER}_SOURCES "${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c" CACHE INTERNAL "${UA_GEN_DT_NAME} source files")
    set(UA_${GEN_NAME_UPPER}_HEADERS "${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h;${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated_handling.h"
        CACHE INTERNAL "${UA_GEN_DT_NAME} header files")
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
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically attached to the server.
#
#   Arguments taking one value:
#
#   NAME            Name of the nodeset, e.g. 'di'
#   [TYPES_ARRAY]   Optional name of the types array containing the custom datatypes of this node set.
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is '${PROJECT_BINARY_DIR}/src_generated'
#   [IGNORE]        Optional file containing a list of node ids which should be ignored. The file should have one id per line.
#   [TARGET_PREFIX] Optional prefix for the resulting target. Default `open62541-generator`
#   [BLACKLIST]     Blacklist file passed as --blacklist to the nodeset compiler. All the given nodes will be removed from the generated
#                   nodeset, including all the references to and from that node. The format is a node id per line.
#                   Supported formats: "i=123" (for NS0), "ns=2;s=asdf" (matches NS2 in that specific file), or recommended
#                   "ns=http://opcfoundation.org/UA/DI/;i=123" namespace index independent node id
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
    find_package(Python3 REQUIRED)

    set(options INTERNAL AUTOLOAD)
    set(oneValueArgs NAME TYPES_ARRAY OUTPUT_DIR IGNORE TARGET_PREFIX BLACKLIST FILES_BSD)
    set(multiValueArgs FILE DEPENDS_TYPES DEPENDS_NS DEPENDS_TARGET)
    cmake_parse_arguments(UA_GEN_NS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(NOT DEFINED open62541_TOOLS_DIR)
        message(FATAL_ERROR "open62541_TOOLS_DIR must point to the open62541 tools directory")
    endif()

    # ------ Argument checking -----
    if(NOT UA_GEN_NS_NAME OR "${UA_GEN_NS_NAME}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeset function requires a value for the NAME argument")
    endif()

    if(NOT UA_GEN_NS_FILE OR "${UA_GEN_NS_FILE}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeset function requires a value for the FILE argument")
    endif()

    # Set default value for output dir
    if(NOT UA_GEN_NS_OUTPUT_DIR OR "${UA_GEN_NS_OUTPUT_DIR}" STREQUAL "")
        set(UA_GEN_NS_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    endif()

    # Set default target prefix
    if(NOT UA_GEN_NS_TARGET_PREFIX OR "${UA_GEN_NS_TARGET_PREFIX}" STREQUAL "")
        set(UA_GEN_NS_TARGET_PREFIX "open62541-generator")
    endif()


    # Set blacklist file
    set(GEN_BLACKLIST "")
    set(GEN_BLACKLIST_DEPENDS "")
    if(UA_GEN_NS_BLACKLIST)
        set(GEN_BLACKLIST "--blacklist=${UA_GEN_NS_BLACKLIST}")
        set(GEN_BLACKLIST_DEPENDS "${UA_GEN_NS_BLACKLIST}")
    endif()

    # Set bsd files
    set(GEN_BSB "")
    set(GEN_BSD_DEPENDS "")
    if(UA_GEN_NS_FILES_BSD)
        foreach(f ${UA_GEN_NS_FILES_BSD})
            set(GEN_BSD ${GEN_BSD} "--bsd=${f}")
        endforeach()
        set(GEN_BSD_DEPENDS "${UA_GEN_NS_FILES_BSD}")
    endif()

    # ------ Add custom command and target -----

    set(GEN_INTERNAL_HEADERS "")
    if (UA_GEN_NS_INTERNAL)
        set(GEN_INTERNAL_HEADERS "--internal-headers")
    endif()

    set(GEN_NS0 "")
    set(TARGET_SUFFIX "ns-${UA_GEN_NS_NAME}")
    set(FILE_SUFFIX "_${UA_GEN_NS_NAME}_generated")
    string(REPLACE "-" "_" FILE_SUFFIX ${FILE_SUFFIX})

    if ("${UA_GEN_NS_NAME}" STREQUAL "ns0")
        set(TARGET_SUFFIX "namespace")
        set(FILE_SUFFIX "0_generated")
    endif()

    set(GEN_IGNORE "")
    if (UA_GEN_NS_IGNORE)
        set(GEN_IGNORE "--ignore=${UA_GEN_NS_IGNORE}")
    endif()

    set(TYPES_ARRAY_LIST "")
    foreach(f ${UA_GEN_NS_DEPENDS_TYPES})
        # Replace dash with underscore to make valid c literal
        string(REPLACE "-" "_" TYPE_ARRAY ${f})

        set(TYPES_ARRAY_LIST ${TYPES_ARRAY_LIST} "--types-array=${TYPE_ARRAY}")
    endforeach()
    if(UA_GEN_NS_TYPES_ARRAY)
        # Replace dash with underscore to make valid c literal
        string(REPLACE "-" "_" TYPE_ARRAY ${UA_GEN_NS_TYPES_ARRAY})
        set(TYPES_ARRAY_LIST ${TYPES_ARRAY_LIST} "--types-array=${TYPE_ARRAY}")
    endif()

    set(DEPENDS_FILE_LIST "")
    foreach(f ${UA_GEN_NS_DEPENDS_NS})
        set(DEPENDS_FILE_LIST ${DEPENDS_FILE_LIST} "--existing=${f}")
    endforeach()
    set(FILE_LIST "")
    foreach(f ${UA_GEN_NS_FILE})
        set(FILE_LIST ${FILE_LIST} "--xml=${f}")
    endforeach()

    # Make sure that the output directory exists
    if(NOT EXISTS ${UA_GEN_NS_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${UA_GEN_NS_OUTPUT_DIR})
    endif()

    add_custom_command(OUTPUT ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c
                       ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h
                       COMMAND ${Python3_EXECUTABLE} ${open62541_TOOLS_DIR}/nodeset_compiler/nodeset_compiler.py
                       ${GEN_INTERNAL_HEADERS}
                       ${GEN_NS0}
                       ${GEN_BIN_SIZE}
                       ${GEN_IGNORE}
                       ${GEN_BLACKLIST}
                       ${GEN_BSD}
                       ${TYPES_ARRAY_LIST}
                       ${DEPENDS_FILE_LIST}
                       ${FILE_LIST}
                       ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}
                       DEPENDS
                       ${open62541_TOOLS_DIR}/nodeset_compiler/nodeset_compiler.py
                       ${open62541_TOOLS_DIR}/nodeset_compiler/nodes.py
                       ${open62541_TOOLS_DIR}/nodeset_compiler/nodeset.py
                       ${open62541_TOOLS_DIR}/nodeset_compiler/datatypes.py
                       ${open62541_TOOLS_DIR}/nodeset_compiler/backend_open62541.py
                       ${open62541_TOOLS_DIR}/nodeset_compiler/backend_open62541_nodes.py
                       ${open62541_TOOLS_DIR}/nodeset_compiler/backend_open62541_datatypes.py
                       ${UA_GEN_NS_FILE}
                       ${UA_GEN_NS_DEPENDS_NS}
                       ${GEN_BLACKLIST_DEPENDS}
                       ${GEN_BSD_DEPENDS}
                       )

    if(NOT TARGET ${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX})
        add_custom_target(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}
                          DEPENDS
                          ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c
                          ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h)
    endif()

    if(UA_GEN_NS_AUTOLOAD)
        if(NOT UA_ENABLE_NODESET_INJECTOR)
            message(WARNING "The AUTOLOAD flag is set. However, the Nodesetinjector feature isn't enabled.")
        else()
            if(NOT TARGET ${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}-autoinjection)
                add_dependencies(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX} open62541-generator-nodesetinjector)
                add_custom_target(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}-autoinjection
                                  COMMAND ${Python3_EXECUTABLE} ${open62541_TOOLS_DIR}/nodeset_injector/generate_nodesetinjector_content.py
                                  ${CMAKE_BINARY_DIR}/src_generated/open62541/nodesetinjector
                                  "namespace${FILE_SUFFIX}"
                                  DEPENDS
                                  ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c
                                  ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h
                                  )
                set_source_files_properties(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}-autoinjection PROPERTIES SYMBOLIC "true")
                add_dependencies(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX} ${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}-autoinjection)

                # The dependency ensures that the generated code is in the correct order in the nodeset injector and
                # that the required namespaces are loaded first. Otherwise it can happen that e.g. machinery is loaded before di,
                # which does not work because machinery is based on di.
                foreach(DEPEND ${UA_GEN_NS_DEPENDS_TARGET})
                    string(FIND ${DEPEND} "open62541-generator-ns" POS)
                    if(POS GREATER_EQUAL 0)
                        add_dependencies(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}-autoinjection ${DEPEND}-autoinjection)
                    endif()
                endforeach()

                list(APPEND UA_NODESETINJECTOR_GENERATORS ${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX})
                set(UA_NODESETINJECTOR_GENERATORS ${UA_NODESETINJECTOR_GENERATORS} PARENT_SCOPE)

                list(APPEND UA_NODESETINJECTOR_SOURCE_FILES  ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c)
                set(UA_NODESETINJECTOR_SOURCE_FILES ${UA_NODESETINJECTOR_SOURCE_FILES} PARENT_SCOPE)

                list(APPEND UA_NODESETINJECTOR_HEADER_FILES  ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h)
                set(UA_NODESETINJECTOR_HEADER_FILES ${UA_NODESETINJECTOR_HEADER_FILES} PARENT_SCOPE)
            endif()
        endif()
    endif()

    if (UA_GEN_NS_DEPENDS_TARGET)
        add_dependencies(${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX} ${UA_GEN_NS_DEPENDS_TARGET})
    endif()

    string(REPLACE "-" "_" UA_GEN_NS_NAME ${UA_GEN_NS_NAME})
    string(TOUPPER "${UA_GEN_NS_NAME}" GEN_NAME_UPPER)

    set_property(GLOBAL PROPERTY "UA_GEN_NS_DEPENDS_FILE_${UA_GEN_NS_NAME}" ${UA_GEN_NS_DEPENDS_NS} ${UA_GEN_NS_FILE})
    set_property(GLOBAL PROPERTY "UA_GEN_NS_DEPENDS_TYPES_${UA_GEN_NS_NAME}" ${UA_GEN_NS_DEPENDS_TYPES} ${UA_GEN_NS_TYPES_ARRAY})

    set_property(GLOBAL PROPERTY UA_NODESET_${GEN_NAME_UPPER}_SOURCES ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c)
    set_property(GLOBAL PROPERTY UA_NODESET_${GEN_NAME_UPPER}_HEADERS ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h)
    set_property(GLOBAL PROPERTY UA_NODESET_${GEN_NAME_UPPER}_TARGET ${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX})

    set(UA_NODESET_${GEN_NAME_UPPER}_SOURCES "${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c" CACHE INTERNAL "UA_NODESET_${GEN_NAME_UPPER} source files")
    set(UA_NODESET_${GEN_NAME_UPPER}_HEADERS "${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h" CACHE INTERNAL "UA_NODESET_${GEN_NAME_UPPER} header files")
    set(UA_NODESET_${GEN_NAME_UPPER}_TARGET "${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX}" CACHE INTERNAL "UA_NODESET_${GEN_NAME_UPPER} target")

endfunction()


# --------------- Generate Nodeset and Datatypes ---------------------
#
# Generates C code for the given NodeSet2.xml and Datatype file.
# This C code can be used to initialize the server.
#
# This is a combination of the ua_generate_datatypes, ua_generate_nodeset, and
# ua_generate_nodeid_header macros.
# This function can also be used to just create a nodeset without datatypes by
# omitting the CSV, BSD, and NAMESPACE_MAP parameter.
# If only one of the previous parameters is given, all of them are required.
#
# It is possible to define dependencies of nodesets by using the DEPENDS argument.
# E.g. the PLCOpen nodeset depends on the 'di' nodeset. Thus it is enough to just
# pass 'DEPENDS di' to the function. The 'di' nodeset then first needs to be generated
# with this function or with the ua_generate_nodeset function.
#
# The resulting cmake target will be named like this:
#   open62541-generator-ns-${NAME}
#
# The following arguments are accepted:
#
#   Options:
#
#   INTERNAL        Include internal headers. Required if custom datatypes are added.
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically attached to the server.
#
#   Arguments taking one value:
#
#   NAME            Short name of the nodeset. E.g. 'di'
#   FILE_NS         Path to the NodeSet2.xml file. Multiple values can be passed. These nodesets will be combined into one output.
#
#   [FILE_CSV]      Optional path to the .csv file containing the node ids, e.g. 'OpcUaDiModel.csv'
#   [FILE_BSD]      Optional path to the .bsd file containing the type definitions, e.g. 'Opc.Ua.Di.Types.bsd'. Multiple files can be
#                   passed which will all combined to one resulting code.
#   [BLACKLIST]     Blacklist file passed as --blacklist to the nodeset compiler. All the given nodes will be removed from the generated
#                   nodeset, including all the references to and from that node. The format is a node id per line.
#                   Supported formats: "i=123" (for NS0), "ns=2;s=asdf" (matches NS2 in that specific file), or recommended
#                   "ns=http://opcfoundation.org/UA/DI/;i=123" namespace index independent node id
#   [TARGET_PREFIX] Optional prefix for the resulting targets. Default `open62541-generator`
#
#   Arguments taking multiple values:
#   [NAMESPACE_MAP] [Deprecated]Array of Namespace index mappings to indicate the final namespace index of a namespace uri when the server is started.
#                   This parameter is mandatory if FILE_CSV or FILE_BSD is set.
#   [IMPORT_BSD]    Optional combination of types array and path to the .bsd file containing additional type definitions referenced by
#                   the FILES_BSD files. The value is separated with a hash sign, i.e.
#                   'UA_TYPES#${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.Types.bsd'
#                   Multiple files can be passed which will all be imported.
#   [DEPENDS]       Optional list of nodeset names on which this nodeset depends. These names must match any name from a previous
#                   call to this funtion. E.g. 'di' if you are generating the 'plcopen' nodeset
#
#
function(ua_generate_nodeset_and_datatypes)
    find_package(Python3 REQUIRED)

    set(options INTERNAL AUTOLOAD)
    set(oneValueArgs NAME FILE_NS FILE_CSV FILE_BSD OUTPUT_DIR TARGET_PREFIX BLACKLIST)
    set(multiValueArgs DEPENDS IMPORT_BSD NAMESPACE_MAP)
    cmake_parse_arguments(UA_GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(NOT DEFINED open62541_TOOLS_DIR)
        message(FATAL_ERROR "open62541_TOOLS_DIR must point to the open62541 tools directory")
    endif()

    if(NOT DEFINED UA_SCHEMA_DIR)
        message(FATAL_ERROR "UA_SCHEMA_DIR must point to the directory that contains the schema files")
    endif()

    # ------ Argument checking -----
    if(DEFINED UA_GEN_NAMESPACE_MAP)
        message(WARNING "NAMESPACE_MAP argument is deprecated and no longer has any effect. The index of the nodeset is set automatically.")
    endif()

    if(NOT UA_GEN_NAME OR "${UA_GEN_NAME}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeset_and_datatypes function requires a value for the NAME argument")
    endif()
    string(TOUPPER "${UA_GEN_NAME}" GEN_NAME_UPPER)

    if(NOT UA_GEN_FILE_NS OR "${UA_GEN_FILE_NS}" STREQUAL "")
        message(FATAL_ERROR "ua_generate_nodeset_and_datatypes function requires a value for the FILE_NS argument")
    endif()

    if((NOT UA_GEN_FILE_CSV OR "${UA_GEN_FILE_CSV}" STREQUAL "") AND
    (NOT "${UA_GEN_FILE_BSD}" STREQUAL ""))
        message(FATAL_ERROR "ua_generate_nodeset_and_datatypes function requires FILE_CSV argument if FILE_BSD is set")
    endif()

    # Set default value for output dir
    if(NOT UA_GEN_OUTPUT_DIR OR "${UA_GEN_OUTPUT_DIR}" STREQUAL "")
        set(UA_GEN_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    endif()
    # Set default target prefix
    if(NOT UA_GEN_TARGET_PREFIX OR "${UA_GEN_TARGET_PREFIX}" STREQUAL "")
        set(UA_GEN_TARGET_PREFIX "open62541-generator")
    endif()

    set(NODESET_DEPENDS_TARGET "")
    set(NODESET_TYPES_ARRAY "UA_TYPES")

    set(NODESET_AUTOLOAD "")
    if (${UA_GEN_AUTOLOAD})
        set(NODESET_AUTOLOAD "AUTOLOAD")
    endif()

    # Extracts the bsd from the xml file and creates a file in the build directory if the bsd file is not specified.
    if("${UA_GEN_FILE_BSD}" STREQUAL "" AND NOT "${UA_GEN_FILE_CSV}" STREQUAL "")
        string(TOUPPER "${UA_GEN_NAME}" BSD_NAME)
        file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/bsd_files_gen")
        execute_process(COMMAND ${Python3_EXECUTABLE} ${open62541_TOOLS_DIR}/generate_bsd.py
                        --xml ${UA_GEN_FILE_NS}
                        ${PROJECT_BINARY_DIR}/bsd_files_gen/Opc.Ua.${BSD_NAME}.Types.bsd)
        if(EXISTS "${PROJECT_BINARY_DIR}/bsd_files_gen/Opc.Ua.${BSD_NAME}.Types.bsd")
            set(UA_GEN_FILE_BSD "${PROJECT_BINARY_DIR}/bsd_files_gen/Opc.Ua.${BSD_NAME}.Types.bsd")
        endif()
    endif()

    if(NOT "${UA_GEN_FILE_BSD}" STREQUAL "")
        set(NAMESPACE_MAP_DEPENDS "${UA_GEN_NAMESPACE_MAP}")

        string(REPLACE "-" "_" GEN_NAME_UPPER ${GEN_NAME_UPPER})
        string(TOUPPER "${GEN_NAME_UPPER}" GEN_NAME_UPPER)
        # Create a list of namespace maps for dependent calls
        if (UA_GEN_DEPENDS AND NOT "${UA_GEN_DEPENDS}" STREQUAL "" )
            foreach(f ${UA_GEN_DEPENDS})
                string(REPLACE "-" "_" DEPENDS_NAME "${f}")
                string(TOUPPER "${DEPENDS_NAME}" DEPENDS_NAME)
                get_property(DEPENDS_NAMESPACE_MAP GLOBAL PROPERTY "UA_GEN_DT_DEPENDS_NAMESPACE_MAP_${DEPENDS_NAME}")

                set(NAMESPACE_MAP_DEPENDS ${NAMESPACE_MAP_DEPENDS} "${DEPENDS_NAMESPACE_MAP}")
            endforeach()
        endif()

        set_property(GLOBAL PROPERTY "UA_GEN_DT_DEPENDS_NAMESPACE_MAP_${GEN_NAME_UPPER}" ${NAMESPACE_MAP_DEPENDS})

        # Generate Datatypes for nodeset
        ua_generate_datatypes(
            NAME "types_${UA_GEN_NAME}"
            TARGET_PREFIX "${UA_GEN_TARGET_PREFIX}"
            TARGET_SUFFIX "types-${UA_GEN_NAME}"
            NAMESPACE_MAP "${NAMESPACE_MAP_DEPENDS}"
            FILE_XML "${UA_GEN_FILE_NS}"
            FILE_CSV "${UA_GEN_FILE_CSV}"
            FILES_BSD "${UA_GEN_FILE_BSD}"
            ${NODESET_AUTOLOAD}
            IMPORT_BSD "${UA_GEN_IMPORT_BSD}"
            OUTPUT_DIR "${UA_GEN_OUTPUT_DIR}"
        )
        set(NODESET_DEPENDS_TARGET "${UA_GEN_TARGET_PREFIX}-types-${UA_GEN_NAME}")
        set(NODESET_TYPES_ARRAY "UA_TYPES_${GEN_NAME_UPPER}")

        ua_generate_nodeid_header(
            NAME "${UA_GEN_NAME}_nodeids"
            ID_PREFIX "${GEN_NAME_UPPER}"
            FILE_CSV "${UA_GEN_FILE_CSV}"
            OUTPUT_DIR "${UA_GEN_OUTPUT_DIR}"
            TARGET_PREFIX "${UA_GEN_TARGET_PREFIX}"
            TARGET_SUFFIX "ids-${UA_GEN_NAME}"
            ${NODESET_AUTOLOAD}
        )
        set(NODESET_DEPENDS_TARGET ${NODESET_DEPENDS_TARGET} "${UA_GEN_TARGET_PREFIX}-ids-${UA_GEN_NAME}")
    else() # Handle nodesets without types in the dependency chain
        if (UA_GEN_DEPENDS AND NOT "${UA_GEN_DEPENDS}" STREQUAL "")
            foreach(f ${UA_GEN_DEPENDS})
                string(REPLACE "-" "_" DEPENDS_NAME "${f}")
                string(TOUPPER "${DEPENDS_NAME}" DEPENDS_NAME)
                get_property(DEPENDS_NAMESPACE_MAP GLOBAL PROPERTY "UA_GEN_DT_DEPENDS_NAMESPACE_MAP_${DEPENDS_NAME}")

                set(NAMESPACE_MAP_DEPENDS ${NAMESPACE_MAP_DEPENDS} "${DEPENDS_NAMESPACE_MAP}")
            endforeach()
        endif()

        # Use namespace 0 as default value
        if (NOT NAMESPACE_MAP_DEPENDS OR "${NAMESPACE_MAP_DEPENDS}" STREQUAL "")
            set(NAMESPACE_MAP_DEPENDS "0:http://opcfoundation.org/UA/")
        endif()

        set_property(GLOBAL PROPERTY "UA_GEN_DT_DEPENDS_NAMESPACE_MAP_${GEN_NAME_UPPER}" ${NAMESPACE_MAP_DEPENDS})
    endif()

    # Create a list of nodesets on which this nodeset depends on
    if (NOT UA_GEN_DEPENDS OR "${UA_GEN_DEPENDS}" STREQUAL "" )
        if(NOT UA_FILE_NS0)
            set(NODESET_DEPENDS "${UA_SCHEMA_DIR}/Opc.Ua.NodeSet2.xml")
        else()
            set(NODESET_DEPENDS "${UA_FILE_NS0}")
        endif()
        set(TYPES_DEPENDS "UA_TYPES")
    else()
        foreach(f ${UA_GEN_DEPENDS})
          if(EXISTS ${f})
            set(NODESET_DEPENDS ${NODESET_DEPENDS} "${f}")
          else()
            string(REPLACE "-" "_" DEPENDS_NAME "${f}")
            get_property(DEPENDS_FILE GLOBAL PROPERTY "UA_GEN_NS_DEPENDS_FILE_${DEPENDS_NAME}")
            if(NOT DEPENDS_FILE OR "${DEPENDS_FILE}" STREQUAL "")
                message(FATAL_ERROR "Nodeset dependency ${f} needs to be generated before ${UA_GEN_NAME}")
            endif()

            set(NODESET_DEPENDS ${NODESET_DEPENDS} "${DEPENDS_FILE}")
            get_property(DEPENDS_TYPES GLOBAL PROPERTY "UA_GEN_NS_DEPENDS_TYPES_${DEPENDS_NAME}")
            set(TYPES_DEPENDS ${TYPES_DEPENDS} "${DEPENDS_TYPES}")
            set(NODESET_DEPENDS_TARGET ${NODESET_DEPENDS_TARGET} "${UA_GEN_TARGET_PREFIX}-ns-${f}")
          endif()
        endforeach()
    endif()


    set(NODESET_INTERNAL "")
    if (${UA_GEN_INTERNAL})
        set(NODESET_INTERNAL "INTERNAL")
    endif()

    ua_generate_nodeset(
        NAME "${UA_GEN_NAME}"
        FILE "${UA_GEN_FILE_NS}"
        TYPES_ARRAY "${NODESET_TYPES_ARRAY}"
        BLACKLIST "${UA_GEN_BLACKLIST}"
        FILES_BSD "${UA_GEN_FILE_BSD}"
        ${NODESET_INTERNAL}
        ${NODESET_AUTOLOAD}
        DEPENDS_TYPES ${TYPES_DEPENDS}
        DEPENDS_NS ${NODESET_DEPENDS}
        DEPENDS_TARGET ${NODESET_DEPENDS_TARGET}
        OUTPUT_DIR "${UA_GEN_OUTPUT_DIR}"
        TARGET_PREFIX "${UA_GEN_TARGET_PREFIX}"
    )

    set(UA_NODESETINJECTOR_GENERATORS ${UA_NODESETINJECTOR_GENERATORS} PARENT_SCOPE)
    set(UA_NODESETINJECTOR_SOURCE_FILES ${UA_NODESETINJECTOR_SOURCE_FILES} PARENT_SCOPE)
    set(UA_NODESETINJECTOR_HEADER_FILES ${UA_NODESETINJECTOR_HEADER_FILES} PARENT_SCOPE)
endfunction()
