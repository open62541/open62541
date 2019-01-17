# Find CHECK libraries
#
# This module defines:
#  CHECK_FOUND - system has check
#  CHECK_INCLUDE_DIRS - the check include directory
#  CHECK_LIBRARIES - check library
#
# If you have libcheck installed in a non-standard place, you can define
# CHECK_PREFIX to tell cmake where it is.

if(CHECK_PREFIX)
    set(CHECK_PREFIX_INC "${CHECK_PREFIX}/include")
    set(CHECK_PREFIX_LIB "${CHECK_PREFIX}/lib")
endif()

find_path(CHECK_INCLUDE_DIRS check.h "${CHECK_PREFIX_INC}")
find_library(CHECK_LIBRARY check HINTS "${CHECK_PREFIX_LIB}")

if(MSVC)

    find_library(COMPAT_LIBRARY compat HINTS "${CHECK_PREFIX_LIB}")
    set(CHECK_LIBRARIES "${CHECK_LIBRARY}" "${COMPAT_LIBRARY}")


    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(CHECK DEFAULT_MSG	CHECK_INCLUDE_DIRS CHECK_LIBRARIES)

    mark_as_advanced(CHECK_INCLUDE_DIRS CHECK_LIBRARIES)
else()

    INCLUDE( FindPkgConfig )

    IF ( Check_FIND_REQUIRED )
        SET( _pkgconfig_REQUIRED "REQUIRED" )
    ELSE( Check_FIND_REQUIRED )
        SET( _pkgconfig_REQUIRED "" )
    ENDIF ( Check_FIND_REQUIRED )

    IF ( CHECK_MIN_VERSION )
        PKG_SEARCH_MODULE( CHECK ${_pkgconfig_REQUIRED} check>=${CHECK_MIN_VERSION} )
    ELSE ( CHECK_MIN_VERSION )
        PKG_SEARCH_MODULE( CHECK ${_pkgconfig_REQUIRED} check )
    ENDIF ( CHECK_MIN_VERSION )

    # Look for CHECK include dir and libraries
    IF( NOT CHECK_FOUND AND NOT PKG_CONFIG_FOUND )

        SET(CHECK_LIBRARIES "CHECK_LIBRARIES-NOTFOUND")
        FIND_LIBRARY( SUBUNIT_LIBRARY NAMES subunit HINTS "${CHECK_PREFIX_LIB}")

        IF ( CHECK_INCLUDE_DIRS AND CHECK_LIBRARY )
            SET(CHECK_LIBRARIES "${CHECK_LIBRARY}")
            if ( SUBUNIT_LIBRARY )
                LIST(APPEND CHECK_LIBRARIES "${SUBUNIT_LIBRARY}")
            endif()
            SET( CHECK_FOUND 1 )
            IF ( NOT Check_FIND_QUIETLY )
                MESSAGE ( STATUS "Found CHECK: ${CHECK_LIBRARIES}" )
            ENDIF ( NOT Check_FIND_QUIETLY )
        ELSE ( CHECK_INCLUDE_DIRS AND CHECK_LIBRARIES )
            IF ( Check_FIND_REQUIRED )
                MESSAGE( FATAL_ERROR "Could NOT find CHECK" )
            ELSE ( Check_FIND_REQUIRED )
                IF ( NOT Check_FIND_QUIETLY )
                    MESSAGE( STATUS "Could NOT find CHECK" )
                ENDIF ( NOT Check_FIND_QUIETLY )
            ENDIF ( Check_FIND_REQUIRED )
        ENDIF ( CHECK_INCLUDE_DIRS AND CHECK_LIBRARIES )
    ENDIF( NOT CHECK_FOUND AND NOT PKG_CONFIG_FOUND )

    # Hide advanced variables from CMake GUIs
    MARK_AS_ADVANCED( CHECK_INCLUDE_DIRS CHECK_LIBRARIES )
endif()
