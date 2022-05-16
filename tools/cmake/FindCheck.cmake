# - Try to find the CHECK libraries
#  Once done this will define
#
#  CHECK_FOUND - system has check
#  CHECK_INCLUDE_DIR - the check include directory
#  CHECK_LIBRARIES - check library
#
#  This configuration file for finding libcheck is originally from
#  the opensync project. The originally was downloaded from here:
#  opensync.org/browser/branches/3rd-party-cmake-modules/modules/FindCheck.cmake
#
#  Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#  Copyright (c) 2007 Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(WIN32)
  # Manually define CHECK_INSTALL_DIR if vcpkg is not used
  if(DEFINED VCPKG_INSTALLED_DIR)
    set(CHECK_INSTALL_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    # find_package(check CONFIG REQUIRED)
  endif()
else()
    # Take care about check.pc settings
    find_package(PkgConfig REQUIRED)
    PKG_SEARCH_MODULE( CHECK check )
endif()

# Look for CHECK include dir and libraries
IF( NOT CHECK_FOUND )
	IF ( CHECK_INSTALL_DIR )
		MESSAGE ( STATUS "Using override CHECK_INSTALL_DIR to find check" )
		SET ( CHECK_INCLUDE_DIR  "${CHECK_INSTALL_DIR}/include" )
		FIND_LIBRARY( CHECK_LIBRARIES NAMES check PATHS "${CHECK_INSTALL_DIR}/lib" )
	ELSE ( CHECK_INSTALL_DIR )
		FIND_PATH( CHECK_INCLUDE_DIR check.h )
		FIND_LIBRARY( CHECK_LIBRARIES NAMES check )
	ENDIF ( CHECK_INSTALL_DIR )

	IF ( CHECK_INCLUDE_DIR AND CHECK_LIBRARIES )
		SET( CHECK_FOUND 1 )
		IF ( NOT Check_FIND_QUIETLY )
			MESSAGE ( STATUS "Found CHECK: ${CHECK_LIBRARIES}" )
		ENDIF ( NOT Check_FIND_QUIETLY )
	ELSE ( CHECK_INCLUDE_DIR AND CHECK_LIBRARIES )
		IF ( Check_FIND_REQUIRED )
			MESSAGE( FATAL_ERROR "Could NOT find CHECK" )
		ELSE ( Check_FIND_REQUIRED )
			IF ( NOT Check_FIND_QUIETLY )
				MESSAGE( STATUS "Could NOT find CHECK" )	
			ENDIF ( NOT Check_FIND_QUIETLY )
		ENDIF ( Check_FIND_REQUIRED )
	ENDIF ( CHECK_INCLUDE_DIR AND CHECK_LIBRARIES )
ENDIF( NOT CHECK_FOUND )

# Hide advanced variables from CMake GUIs
MARK_AS_ADVANCED( CHECK_INCLUDE_DIR CHECK_LIBRARIES )
