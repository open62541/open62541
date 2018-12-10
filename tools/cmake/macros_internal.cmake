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
