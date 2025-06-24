macro(set_default VAR DEFAULT)
    if(NOT DEFINED ${VAR} OR "${${VAR}}" STREQUAL "")
        set(${VAR} "${DEFAULT}")
    endif()
endmacro()

macro(set_parent VAR)
    set(${VAR} "${${VAR}}" PARENT_SCOPE)
endmacro()

# In a local dev environment, manually set the variables from open62541Config.cmake
set_default(open62541_TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
set_parent(open62541_TOOLS_DIR)
set_default(open62541_NS0_NODESETS ${UA_NS0_NODESET_FILES})
set_parent(open62541_NS0_NODESETS)
set_default(open62541_SCHEMA_DIR ${UA_SCHEMA_DIR})
set_parent(open62541_SCHEMA_DIR)

# Set global variables to find targets and sources of dependencies
function(export_target)
    set(options)
    set(oneValueArgs NAME TARGET)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")
    string(TOUPPER ${ARGV0} arg_NAME)
    string(REPLACE "-" "_" arg_NAME ${arg_NAME})
    set(${arg_NAME}_TARGET ${arg_TARGET} CACHE INTERNAL "")
    set(${arg_NAME}_SOURCES ${arg_SOURCES} CACHE INTERNAL "")
endfunction()

# --------------- Generate NodeIds header ---------------------
#
# Generates header file from .csv which contains defines for every node id to be
# used instead of numeric node ids.
#
# The resulting files will be ${OUTPUT_DIR}/${NAME.h}
# The resulting generator name is ${TARGET_PREFIX}-${TARGET_SUFFIX}
#
# The following arguments are accepted:
#   Options:
#
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically
#                   attached to the server.
#
#   Arguments taking one value:
#
#   NAME            Full name of the generated files, e.g. di_nodeids
#   ID_PREFIX       Prefix for the generated node ID defines, e.g. NS_DI
#   TARGET_SUFFIX   Suffix for the resulting target. e.g. ids-di
#   FILE_CSV        Path to the .csv file containing the node ids, e.g. 'OpcUaDiModel.csv'
#   [TARGET_PREFIX] Optional prefix for the resulting target. Default `open62541-generator`
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is
#                   '${PROJECT_BINARY_DIR}/src_generated/open62541'

function(ua_generate_nodeid_header)
    find_package(Python3 REQUIRED)
    set(options AUTOLOAD)
    set(oneValueArgs NAME ID_PREFIX OUTPUT_DIR FILE_CSV TARGET_SUFFIX TARGET_PREFIX)
    set(multiValueArgs)
    cmake_parse_arguments(UA_GEN_ID "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Argument checking
    set_default(UA_GEN_ID_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    set_default(UA_GEN_ID_TARGET_PREFIX "open62541-generator")
    if(NOT UA_GEN_ID_TARGET_SUFFIX OR "${UA_GEN_ID_TARGET_SUFFIX}" STREQUAL "")
        message(FATAL_ERROR "TARGET_SUFFIX argument required")
    endif()

    # Make sure that the output directory exists
    if(NOT EXISTS ${UA_GEN_ID_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${UA_GEN_ID_OUTPUT_DIR})
    endif()

    # Replace dash with underscore to make valid c literal
    string(REPLACE "-" "_" UA_GEN_ID_NAME ${UA_GEN_ID_NAME})

    # Command generating the header containing defines for all NodeIds
    add_custom_command(COMMAND ${Python3_EXECUTABLE}
                               ${open62541_TOOLS_DIR}/generate_nodeid_header.py
                               ${UA_GEN_ID_FILE_CSV}
                               ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}
                               ${UA_GEN_ID_ID_PREFIX}
                       OUTPUT  ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}.h
                       DEPENDS ${open62541_TOOLS_DIR}/generate_nodeid_header.py
                               ${UA_GEN_ID_FILE_CSV})

    # Add the target depending on the generated file
    set(TARGET_NAME "${UA_GEN_ID_TARGET_PREFIX}-${UA_GEN_ID_TARGET_SUFFIX}")
    if(NOT TARGET ${TARGET_NAME})
        add_custom_target(${TARGET_NAME} DEPENDS ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}.h)
        export_target(UA_${UA_GEN_ID_NAME}
                      TARGET ${TARGET_NAME}
                      SOURCES ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}.h)
    endif()

    # Add to the injector list
    if(UA_GEN_ID_AUTOLOAD)
        if(NOT UA_ENABLE_NODESET_INJECTOR)
            message(FATAL_ERROR "The AUTOLOAD flag requires the Nodesetinjector feature to be enabled")
        endif()
        add_dependencies(open62541-generator-nodesetinjector ${TARGET_NAME})
        list(APPEND UA_NODESETINJECTOR_SOURCE_FILES ${UA_GEN_ID_OUTPUT_DIR}/${UA_GEN_ID_NAME}.h)
        set_parent(UA_NODESETINJECTOR_SOURCE_FILES)
    endif()
endfunction()

# --------------- Generate Datatypes ---------------------
#
# Generates Datatype definition based on the .csv and .bsd files of a nodeset.
# The result of the generation will be C Code which can be compiled with the
# rest of the stack. Some nodesets come with custom datatypes. These datatype
# structures first need to be generated so that the nodeset can use these types.
#
# The resulting files will be put into ${OUTPUT_DIR} with the names:
# - ${NAME}_generated.c
# - ${NAME}_generated.h
# - ${NAME}_generated.rst (optional / depends on GEN_DOC)
#
# The resulting cmake target is named ${TARGET_PREFIX}-${TARGET_SUFFIX}
#
# The following arguments are accepted:
#   Options:
#
#   [BUILTIN]       Optional argument. If given, then builtin types will be generated.
#   [INTERNAL]      Optional argument. If given, then the given types file is seen as
#                   internal file (e.g. does not require a .csv)
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically
#                   attached to the server.
#   [GEN_DOC]       Optional argument. If given, a .rst file for documenting the
#                   generated datatypes is generated.
#
#   Arguments taking one value:
#
#   NAME            Full name of the generated files, e.g. ua_types_di
#   TARGET_SUFFIX   Suffix for the resulting target. e.g. types-di
#   FILE_CSV        Path to the .csv file containing the node ids, e.g. 'OpcUaDiModel.csv'
#   [TARGET_PREFIX] Optional prefix for the resulting target. Default
#                   `open62541-generator`
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is
#                   '${PROJECT_BINARY_DIR}/src_generated/open62541'
#
#   Arguments taking multiple values:
#
#   FILES_BSD       Path to the .bsd file containing the type definitions, e.g.
#                   'Opc.Ua.Di.Types.bsd'. Multiple files can be passed which
#                   will all combined to one resulting code.
#   IMPORT_BSD      Combination of types array and path to the .bsd file containing
#                   additional type definitions referenced by the FILES_BSD
#                   files. The value is separated with a hash sign, i.e.
#                   'UA_TYPES#${open62541_SCHEMA_DIR}/Opc.Ua.Types.bsd' Multiple files
#                   can be passed which will all be imported.
#   [FILES_SELECTED] Optional path to a simple text file which contains a list
#                   of types which should be included in the generation. The
#                   file should contain one type per line. Multiple files can be
#                   passed to this argument.

function(ua_generate_datatypes)
    find_package(Python3 REQUIRED)
    set(options BUILTIN INTERNAL AUTOLOAD GEN_DOC)
    set(oneValueArgs NAME TARGET_SUFFIX TARGET_PREFIX OUTPUT_DIR FILE_XML FILE_CSV)
    set(multiValueArgs FILES_BSD IMPORT_BSD FILES_SELECTED)
    cmake_parse_arguments(UA_GEN_DT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Argument checking
    set_default(UA_GEN_DT_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    set_default(UA_GEN_DT_TARGET_PREFIX "open62541-generator")
    if(NOT UA_GEN_DT_NAME OR "${UA_GEN_DT_NAME}" STREQUAL "")
        message(FATAL_ERROR "NAME argument required")
    endif()
    if(NOT UA_GEN_DT_TARGET_SUFFIX OR "${UA_GEN_DT_TARGET_SUFFIX}" STREQUAL "")
        message(FATAL_ERROR "TARGET_SUFFIX argument required")
    endif()
    if(NOT UA_GEN_DT_FILE_CSV OR "${UA_GEN_DT_FILE_CSV}" STREQUAL "")
        message(FATAL_ERROR "FILE_CSV argument required")
    endif()
    if(NOT UA_GEN_DT_FILES_BSD OR "${UA_GEN_DT_FILES_BSD}" STREQUAL "")
        message(FATAL_ERROR "FILES_BSD argument required")
    endif()

    # Make sure that the output directory exists
    if(NOT EXISTS ${UA_GEN_DT_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${UA_GEN_DT_OUTPUT_DIR})
    endif()

    # Replace dash with underscore to make valid c literal
    string(REPLACE "-" "_" UA_GEN_DT_NAME ${UA_GEN_DT_NAME})

    # Set up the arguments for the command
    set(UA_GEN_DT_NO_BUILTIN "--no-builtin")
    if(UA_GEN_DT_BUILTIN)
        set(UA_GEN_DT_NO_BUILTIN "")
    endif()

    set(UA_GEN_DOC_ARG "")
    if(UA_GEN_DT_GEN_DOC)
        set(UA_GEN_DOC_ARG "--gen-doc")
    endif()

    set(UA_GEN_DT_INTERNAL_ARG "")
    if(UA_GEN_DT_INTERNAL)
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

    if((MINGW) AND (DEFINED ENV{SHELL}))
        # fix issue 4156 that MINGW will do automatic Windows Path Conversion
        # powershell handles Windows Path correctly
        # MINGW SHELL only accept environment variable with "env"
        set(ARG_CONV_EXCL_ENV env MSYS2_ARG_CONV_EXCL=--import)
    endif()

    set(FILE_XML "")
    if(UA_GEN_DT_FILE_XML)
        set(FILE_XML "--xml=${UA_GEN_DT_FILE_XML}")
    endif()

    # Command generating the DataType code files
    add_custom_command(COMMAND ${ARG_CONV_EXCL_ENV} ${Python3_EXECUTABLE}
                               ${open62541_TOOLS_DIR}/generate_datatypes.py
                               ${SELECTED_TYPES_TMP}
                               ${BSD_FILES_TMP}
                               ${IMPORT_BSD_TMP}
                               ${FILE_XML}
                               --type-csv=${UA_GEN_DT_FILE_CSV}
                               ${UA_GEN_DT_NO_BUILTIN}
                               ${UA_GEN_DT_INTERNAL_ARG}
                               ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}
                               ${UA_GEN_DOC_ARG}
                       OUTPUT  ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
                               ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h
                       DEPENDS ${open62541_TOOLS_DIR}/generate_datatypes.py
                               ${open62541_TOOLS_DIR}/nodeset_compiler/type_parser.py
                               ${UA_GEN_DT_FILES_BSD}
                               ${UA_GEN_DT_FILE_XML}
                               ${UA_GEN_DT_FILE_CSV}
                               ${UA_GEN_DT_FILES_SELECTED})

    # Define the corresponding target
    set(TARGET_NAME ${UA_GEN_DT_TARGET_PREFIX}-${UA_GEN_DT_TARGET_SUFFIX})
    if(NOT TARGET ${TARGET_NAME})
        add_custom_target(${TARGET_NAME}
                          DEPENDS ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
                                  ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h)
        export_target(UA_${UA_GEN_DT_NAME}
                      TARGET ${TARGET_NAME}
                      SOURCES ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.c
                              ${UA_GEN_DT_OUTPUT_DIR}/${UA_GEN_DT_NAME}_generated.h)
    endif()

    # Add to the injector list
    if(UA_GEN_DT_AUTOLOAD)
        if(NOT UA_ENABLE_NODESET_INJECTOR)
            message(FATAL_ERROR "The AUTOLOAD flag requires the Nodesetinjector feature to be enabled")
        endif()
        add_dependencies(open62541-generator-nodesetinjector ${TARGET_NAME})
        list(APPEND UA_NODESETINJECTOR_SOURCE_FILES ${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_GEN_DT_NAME}_generated.c
                                                    ${PROJECT_BINARY_DIR}/src_generated/open62541/${UA_GEN_DT_NAME}_generated.h)
        set_parent(UA_NODESETINJECTOR_SOURCE_FILES)
    endif()
endfunction()

# --------------- Generate Nodeset ---------------------
#
# Generates C code for the given NodeSet2.xml file. This C code can be used to
# initialize the server.
#
# The resulting files will be put into OUTPUT_DIR with the names:
# - ua_namespace_NAME.c
# - ua_namespace_NAME.h
#
# The resulting cmake target will be named like this:
#   {TARGET_PREFIX}-ns-${NAME}
#
# The following arguments are accepted:
#   Options:
#
#   [INTERNAL]      Optional argument. If given, then the generated node set code
#                   will use internal headers.
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically attached to the server.
#
#   Arguments taking one value:
#
#   NAME            Name of the nodeset, e.g. 'di'
#   [TYPES_ARRAY]   Optional name of the types array containing the custom
#                   datatypes of this node set.
#   [OUTPUT_DIR]    Optional target directory for the generated files. Default is
#                   '${PROJECT_BINARY_DIR}/src_generated/open62541'
#   [IGNORE]        Optional file containing a list of node ids which should be
#                   ignored. The file should have one id per line.
#   [TARGET_PREFIX] Optional prefix for the resulting target. Default `open62541-generator`
#   [BLACKLIST]     Blacklist file passed as --blacklist to the nodeset compiler.
#                   All the given nodes will be removed from the generated
#                   nodeset, including all the references to and from that node.
#                   The format is a node id per line. Supported formats: "i=123"
#                   (for NS0), "ns=2;s=asdf" (matches NS2 in that specific
#                   file), or recommended "ns=http://opcfoundation.org/UA/DI/;i=123"
#
#   Arguments taking multiple values:
#
#   FILE              Path to the NodeSet2.xml file. Multiple values can be passed. These
#                     nodesets will be combined into one output.
#   [DEPENDS_TYPES]   Optional list of types array which match with the DEPENDS_NS
#                     node sets. e.g. 'UA_TYPES;UA_TYPES_DI'
#   [DEPENDS_NS]      Optional list of NodeSet2.xml files which are a dependency of
#                     this node set.
#   [DEPENDS_TARGET]  Optional list of CMake targets this nodeset depends on.

function(ua_generate_nodeset)
    find_package(Python3 REQUIRED)
    set(options INTERNAL AUTOLOAD)
    set(oneValueArgs NAME TYPES_ARRAY OUTPUT_DIR IGNORE TARGET_PREFIX BLACKLIST FILES_BSD)
    set(multiValueArgs FILE DEPENDS_TYPES DEPENDS_NS DEPENDS_TARGET)
    cmake_parse_arguments(UA_GEN_NS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Argument checking
    set_default(UA_GEN_NS_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    set_default(UA_GEN_NS_TARGET_PREFIX "open62541-generator")
    if(NOT UA_GEN_NS_NAME OR "${UA_GEN_NS_NAME}" STREQUAL "")
        message(FATAL_ERROR "NAME argument required")
    endif()
    if(NOT UA_GEN_NS_FILE OR "${UA_GEN_NS_FILE}" STREQUAL "")
        message(FATAL_ERROR "FILE argument required")
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

    # Make sure that the output directory exists
    if(NOT EXISTS ${UA_GEN_NS_OUTPUT_DIR})
        file(MAKE_DIRECTORY ${UA_GEN_NS_OUTPUT_DIR})
    endif()

    # Set up the command
    set(GEN_INTERNAL_HEADERS "")
    if(UA_GEN_NS_INTERNAL)
        set(GEN_INTERNAL_HEADERS "--internal-headers")
    endif()

    set(GEN_NS0 "")
    set(TARGET_SUFFIX "ns-${UA_GEN_NS_NAME}")
    set(FILE_SUFFIX "_${UA_GEN_NS_NAME}_generated")
    string(REPLACE "-" "_" FILE_SUFFIX ${FILE_SUFFIX})

    if("${UA_GEN_NS_NAME}" STREQUAL "ns0")
        set(TARGET_SUFFIX "namespace")
        set(FILE_SUFFIX "0_generated")
    endif()

    set(GEN_IGNORE "")
    if(UA_GEN_NS_IGNORE)
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
    else()
        set(UA_GEN_NS_TYPES_ARRAY "")
    endif()

    set(DEPENDS_FILE_LIST "")
    foreach(f ${UA_GEN_NS_DEPENDS_NS})
        set(DEPENDS_FILE_LIST ${DEPENDS_FILE_LIST} "--existing=${f}")
    endforeach()
    set(FILE_LIST "")
    foreach(f ${UA_GEN_NS_FILE})
        set(FILE_LIST ${FILE_LIST} "--xml=${f}")
    endforeach()

    add_custom_command(COMMAND ${Python3_EXECUTABLE}
                               ${open62541_TOOLS_DIR}/nodeset_compiler/nodeset_compiler.py
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
                       OUTPUT  ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c
                               ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h
                       DEPENDS ${open62541_TOOLS_DIR}/nodeset_compiler/nodeset_compiler.py
                               ${open62541_TOOLS_DIR}/nodeset_compiler/nodes.py
                               ${open62541_TOOLS_DIR}/nodeset_compiler/nodeset.py
                               ${open62541_TOOLS_DIR}/nodeset_compiler/datatypes.py
                               ${open62541_TOOLS_DIR}/nodeset_compiler/backend_open62541.py
                               ${UA_GEN_NS_FILE}
                               ${UA_GEN_NS_DEPENDS_NS}
                               ${GEN_BLACKLIST_DEPENDS}
                               ${GEN_BSD_DEPENDS})

    # Collect file names of type/nodeid sources
    string(TOUPPER ${UA_GEN_NS_NAME} UA_GEN_NS_NAME_UPPER)
    string(REPLACE "-" "_" UA_GEN_NS_NAME_UPPER ${UA_GEN_NS_NAME_UPPER})
    set(TARGET_SOURCES ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.c;
                       ${UA_GEN_NS_OUTPUT_DIR}/namespace${FILE_SUFFIX}.h)
    if(DEFINED UA_NODEIDS_${UA_GEN_NS_NAME_UPPER}_SOURCES)
        list(APPEND TARGET_SOURCES ${UA_NODEIDS_${UA_GEN_NS_NAME_UPPER}_SOURCES})
    endif()
    if(DEFINED UA_TYPES_${UA_GEN_NS_NAME_UPPER}_SOURCES)
        list(APPEND TARGET_SOURCES ${UA_TYPES_${UA_GEN_NS_NAME_UPPER}_SOURCES})
    endif()

    set(TARGET_NAME ${UA_GEN_NS_TARGET_PREFIX}-${TARGET_SUFFIX})
    if(NOT TARGET ${TARGET_NAME})
        # Generate the target
        add_custom_target(${TARGET_NAME} DEPENDS ${TARGET_SOURCES})

        # Export target and source names and also the nodeset
        export_target(UA_NODESET_${UA_GEN_NS_NAME}
                      TARGET ${TARGET_NAME} SOURCES ${TARGET_SOURCES})

        # Export global variable for the nodeset xml files and types used for the target
        list(APPEND UA_GEN_NS_DEPENDS_NS ${UA_GEN_NS_FILE})
        list(REMOVE_DUPLICATES UA_GEN_NS_DEPENDS_NS)
        set(UA_NODESET_${UA_GEN_NS_NAME_UPPER}_XML ${UA_GEN_NS_DEPENDS_NS} CACHE INTERNAL "")
        list(APPEND UA_GEN_NS_TYPES_ARRAY ${UA_GEN_NS_DEPENDS_TYPES})
        list(REMOVE_DUPLICATES UA_GEN_NS_TYPES_ARRAY)
        set(UA_NODESET_${UA_GEN_NS_NAME_UPPER}_TYPES ${UA_GEN_NS_TYPES_ARRAY} CACHE INTERNAL "")
    endif()

    # Collect target dependencies
    if(UA_GEN_NS_DEPENDS_TARGET)
        foreach(DEPEND_TARGET ${UA_GEN_NS_DEPENDS_TARGET})
            # Is this a target name? If not, read from variable ${DEPEND_TARGET}_TARGET.
            if(NOT TARGET ${DEPEND_TARGET})
                string(TOUPPER ${DEPEND_TARGET} DEPEND_TARGET)
                string(REPLACE "-" "_" DEPEND_TARGET UA_NODESET_${DEPEND_TARGET}_TARGET)
                set(DEPEND_TARGET ${${DEPEND_TARGET}})
            endif()
            if(DEPEND_TARGET)
                add_dependencies(${TARGET_NAME} ${DEPEND_TARGET})
            endif()
        endforeach()
    endif()

    if(UA_GEN_NS_AUTOLOAD)
        if(NOT UA_ENABLE_NODESET_INJECTOR)
            message(FATAL_ERROR "The AUTOLOAD flag requires the Nodesetinjector feature to be enabled")
        endif()
        add_dependencies(open62541-generator-nodesetinjector ${TARGET_NAME})
        list(APPEND UA_NODESETINJECTOR_SOURCE_FILES ${TARGET_SOURCES})
        set_parent(UA_NODESETINJECTOR_SOURCE_FILES)
    endif()
endfunction()

# --------------- Generate Nodeset and Datatypes ---------------------
#
# Generates C code for the given NodeSet2.xml and Datatype file. This C code can
# be used to initialize the server.
#
# This is a combination of the ua_generate_datatypes, ua_generate_nodeset, and
# ua_generate_nodeid_header macros. This function can also be used to just
# create a nodeset without datatypes by omitting the CSV and BSD parameter. If
# only one of the previous parameters is given, all of them are required.
#
# It is possible to define dependencies of nodesets by using the DEPENDS
# argument. E.g. the PLCOpen nodeset depends on the 'di' nodeset. Thus it is
# enough to just pass 'DEPENDS di' to the function. The 'di' nodeset then first
# needs to be generated with this function or with the ua_generate_nodeset
# function.
#
# The resulting cmake target will be named ${TARGET_PREFIX}-ns-${NAME}
#
# The following arguments are accepted:
#
#   Options:
#
#   INTERNAL        Include internal headers. Required if custom datatypes are added.
#   [AUTOLOAD]      Optional argument. If given, the nodeset is automatically
#                   attached to the server.
#
#   Arguments taking one value:
#
#   NAME            Short name of the nodeset. E.g. 'di'
#   FILE_NS         Path to the NodeSet2.xml file. Multiple values can be passed. These
#                   nodesets will be combined into one output.
#   [FILE_CSV]      Optional path to the .csv file containing the node ids, e.g.
#                   'OpcUaDiModel.csv'
#   [FILE_BSD]      Optional path to the .bsd file containing the type definitions,
#                   e.g. 'Opc.Ua.Di.Types.bsd'. Multiple files can be passed
#                   which will all combined to one resulting code.
#   [BLACKLIST]     Blacklist file passed as --blacklist to the nodeset compiler.
#                   All the given nodes will be removed from the generated
#                   nodeset, including all the references to and from that node.
#                   The format is a node id per line. Supported formats: "i=123"
#                   (for NS0), "ns=2;s=asdf" (matches NS2 in that specific
#                   file), or recommended "ns=http://opcfoundation.org/UA/DI/;i=123"
#   [TARGET_PREFIX] Optional prefix for the resulting targets. Default
#                   `open62541-generator`
#
#   Arguments taking multiple values:
#
#   [IMPORT_BSD]    Optional combination of types array and path to the .bsd file
#                   containing additional type definitions referenced by the
#                   FILES_BSD files. The value is separated with a hash sign,
#                   i.e.
#                   'UA_TYPES#${PROJECT_SOURCE_DIR}/deps/ua-nodeset/Schema/Opc.Ua.Types.bsd'
#                   Multiple files can be passed which will all be imported.
#   [DEPENDS]       Optional list of nodeset names on which this nodeset depends.
#                   These names must match any name from a previous call to this
#                   funtion. E.g. 'di' if you are generating the 'plcopen'
#                   nodeset

function(ua_generate_nodeset_and_datatypes)
    find_package(Python3 REQUIRED)
    set(options INTERNAL AUTOLOAD)
    set(oneValueArgs NAME FILE_NS FILE_CSV FILE_BSD OUTPUT_DIR TARGET_PREFIX BLACKLIST)
    set(multiValueArgs DEPENDS IMPORT_BSD)
    cmake_parse_arguments(UA_GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Argument checking
    set_default(UA_GEN_OUTPUT_DIR ${PROJECT_BINARY_DIR}/src_generated/open62541)
    set_default(UA_GEN_TARGET_PREFIX "open62541-generator")
    if(NOT UA_GEN_NAME OR "${UA_GEN_NAME}" STREQUAL "")
        message(FATAL_ERROR "NAME argument required")
    endif()
    if(NOT UA_GEN_FILE_NS OR "${UA_GEN_FILE_NS}" STREQUAL "")
        message(FATAL_ERROR "FILE_NS argument required")
    endif()
    if((NOT UA_GEN_FILE_CSV OR "${UA_GEN_FILE_CSV}" STREQUAL "") AND
       (NOT "${UA_GEN_FILE_BSD}" STREQUAL ""))
        message(FATAL_ERROR "FILE_CSV argument required if FILE_BSD is set")
    endif()

    string(TOUPPER "${UA_GEN_NAME}" GEN_NAME_UPPER)
    string(REPLACE "-" "_" GEN_NAME_UPPER ${GEN_NAME_UPPER})

    set(NODESET_AUTOLOAD "")
    if(${UA_GEN_AUTOLOAD})
        set(NODESET_AUTOLOAD "AUTOLOAD")
    endif()

    # If the bsd file is not specified, extract the bsd from the xml file and
    # create a file in the build directory.
    if("${UA_GEN_FILE_BSD}" STREQUAL "" AND NOT "${UA_GEN_FILE_CSV}" STREQUAL "")
        string(TOUPPER "${UA_GEN_NAME}" BSD_NAME)
        file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/bsd_files_gen")
        set(UA_GEN_FILE_BSD_TMP "${PROJECT_BINARY_DIR}/bsd_files_gen/Opc.Ua.${BSD_NAME}.Types.bsd")
        execute_process(COMMAND ${Python3_EXECUTABLE} ${open62541_TOOLS_DIR}/generate_bsd.py
                                --xml ${UA_GEN_FILE_NS} ${UA_GEN_FILE_BSD_TMP})
        if(EXISTS "${UA_GEN_FILE_BSD_TMP}")
            set(UA_GEN_FILE_BSD "${UA_GEN_FILE_BSD_TMP}")
        endif()
    endif()

    set(NODESET_TYPES_ARRAY "UA_TYPES")
    if(NOT "${UA_GEN_FILE_BSD}" STREQUAL "")
        set(NODESET_TYPES_ARRAY "UA_TYPES_${GEN_NAME_UPPER}")
    endif()

    # All nodesets (besides ns0) depend on ns0
    if(NOT ${UA_GEN_NAME} STREQUAL "ns0")
        set(NODESET_DEPENDS ${open62541_NS0_NODESETS})
        set(TYPES_DEPENDS "UA_TYPES")
    endif()

    # Add xml nodesets and types from the dependencies
    foreach(f ${UA_GEN_DEPENDS})
        string(TOUPPER ${f} f)
        string(REPLACE "-" "_" f ${f})
        if(DEFINED UA_NODESET_${f}_XML)
            list(APPEND NODESET_DEPENDS "${UA_NODESET_${f}_XML}")
        endif()
        if(DEFINED UA_NODESET_${f}_TYPES)
            list(APPEND TYPES_DEPENDS "${UA_NODESET_${f}_TYPES}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES NODESET_DEPENDS)
    list(REMOVE_DUPLICATES TYPES_DEPENDS)

    set(NODESET_INTERNAL "")
    if(${UA_GEN_INTERNAL})
        set(NODESET_INTERNAL "INTERNAL")
    endif()

    if(NOT "${UA_GEN_FILE_BSD}" STREQUAL "")
        # Generates target ${UA_GEN_TARGET_PREFIX}-types-${UA_GEN_NAME}
        ua_generate_datatypes(NAME types-${UA_GEN_NAME}
                              TARGET_PREFIX "${UA_GEN_TARGET_PREFIX}"
                              TARGET_SUFFIX "types-${UA_GEN_NAME}"
                              FILE_XML "${UA_GEN_FILE_NS}"
                              FILE_CSV "${UA_GEN_FILE_CSV}"
                              FILES_BSD "${UA_GEN_FILE_BSD}"
                              ${NODESET_AUTOLOAD}
                              IMPORT_BSD "${UA_GEN_IMPORT_BSD}"
                              OUTPUT_DIR "${UA_GEN_OUTPUT_DIR}")

        # Generates target ${UA_GEN_TARGET_PREFIX}-ids-${UA_GEN_NAME}
        ua_generate_nodeid_header(NAME nodeids-${UA_GEN_NAME}
                                  ID_PREFIX "${GEN_NAME_UPPER}"
                                  FILE_CSV "${UA_GEN_FILE_CSV}"
                                  OUTPUT_DIR "${UA_GEN_OUTPUT_DIR}"
                                  TARGET_PREFIX "${UA_GEN_TARGET_PREFIX}"
                                  TARGET_SUFFIX "ids-${UA_GEN_NAME}"
                                  ${NODESET_AUTOLOAD})
    endif()

    # Generates target ${UA_GEN_TARGET_PREFIX}-ns-${UA_GEN_NAME}
    ua_generate_nodeset(NAME "${UA_GEN_NAME}"
                        FILE "${UA_GEN_FILE_NS}"
                        TYPES_ARRAY "${NODESET_TYPES_ARRAY}"
                        BLACKLIST "${UA_GEN_BLACKLIST}"
                        FILES_BSD "${UA_GEN_FILE_BSD}"
                        ${NODESET_INTERNAL}
                        ${NODESET_AUTOLOAD}
                        DEPENDS_TYPES ${TYPES_DEPENDS}
                        DEPENDS_NS ${NODESET_DEPENDS}
                        DEPENDS_TARGET ${UA_GEN_DEPENDS}
                        OUTPUT_DIR "${UA_GEN_OUTPUT_DIR}"
                        TARGET_PREFIX "${UA_GEN_TARGET_PREFIX}")

    set_parent(UA_NODESETINJECTOR_SOURCE_FILES)
endfunction()
