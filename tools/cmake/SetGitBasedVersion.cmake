get_filename_component(VERSION_SRC_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(VERSION_SRC_DIR "${VERSION_SRC_DIR}/..")

find_package(Git)

function(set_open62541_version)

    # Generate a git-describe version string from Git repository tags
    if(GIT_EXECUTABLE AND NOT DEFINED OPEN62541_VERSION)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
            WORKING_DIRECTORY "${VERSION_SRC_DIR}"
            OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
            RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT GIT_DESCRIBE_ERROR_CODE)

            # Example values can be:
            # v1.2
            # v1.2.3
            # v1.2.3-rc1
            # v1.2.3-rc1-dirty
            # v1.2.3-5-g4538abcd
            # v1.2.3-5-g4538abcd-dirty

            set(OPEN62541_VERSION ${GIT_DESCRIBE_VERSION})
        endif()
    endif()

    if(OPEN62541_VERSION)
        STRING(REGEX REPLACE "^(v[0-9\\.]+)(.*)$"
               "\\1"
               GIT_VERSION_NUMBERS
               "${OPEN62541_VERSION}" )
        STRING(REGEX REPLACE "^v([0-9\\.]+)(.*)$"
               "\\2"
               GIT_VERSION_LABEL
               "${OPEN62541_VERSION}" )

        if("${GIT_VERSION_NUMBERS}" MATCHES "^v([0-9]+)(.*)$")
            STRING(REGEX REPLACE "^v([0-9]+)\\.?(.*)$"
                   "\\1"
                   GIT_VER_MAJOR
                   "${GIT_VERSION_NUMBERS}" )
            if("${GIT_VERSION_NUMBERS}" MATCHES "^v([0-9]+)\\.([0-9]+)(.*)$")
                STRING(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)(.*)$"
                       "\\2"
                       GIT_VER_MINOR
                       "${GIT_VERSION_NUMBERS}" )
                if("${GIT_VERSION_NUMBERS}" MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)$")
                    STRING(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)$"
                           "\\3"
                           GIT_VER_PATCH
                           "${GIT_VERSION_NUMBERS}" )
                else()
                    set(GIT_VER_PATCH 0)
                endif()
            else()
                set(GIT_VER_MINOR 0)
                set(GIT_VER_PATCH 0)
            endif()

        else()
            set(GIT_VER_MAJOR 0)
            set(GIT_VER_MINOR 0)
            set(GIT_VER_PATCH 0)
        endif()
        set(OPEN62541_VER_MAJOR ${GIT_VER_MAJOR} PARENT_SCOPE)
        set(OPEN62541_VER_MINOR ${GIT_VER_MINOR} PARENT_SCOPE)
        set(OPEN62541_VER_PATCH ${GIT_VER_PATCH} PARENT_SCOPE)
        set(OPEN62541_VER_LABEL "${GIT_VERSION_LABEL}" PARENT_SCOPE)
        set(OPEN62541_VER_COMMIT ${OPEN62541_VERSION} PARENT_SCOPE)
    else()
        # Final fallback: Just use a bogus version string that is semantically older
        # than anything else and spit out a warning to the developer.
        set(OPEN62541_VERSION "v0.0.0-unknown" PARENT_SCOPE)
        set(OPEN62541_VER_MAJOR 0 PARENT_SCOPE)
        set(OPEN62541_VER_MINOR 0 PARENT_SCOPE)
        set(OPEN62541_VER_PATCH 0 PARENT_SCOPE)
        set(OPEN62541_VER_LABEL "-unknown" PARENT_SCOPE)
        set(OPEN62541_VER_COMMIT "undefined" PARENT_SCOPE)
        message(WARNING "Failed to determine OPEN62541_VERSION from repository tags. Using default version \"${OPEN62541_VERSION}\".")
    endif()
endfunction()
