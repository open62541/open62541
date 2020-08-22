#check environment variable
if("$ENV{PROTON_FOLDER_INCLUDE}")
    set(PROTON_FOLDER_INCLUDE "$ENV{PROTON_FOLDER_INCLUDE}")
endif()
if("$ENV{PROTON_FOLDER_LIBRARY}")
    set(PROTON_FOLDER_LIBRARY "$ENV{PROTON_FOLDER_LIBRARY}")
endif()

find_library(PROTON_LIBRARY qpid-proton HINTS ${PROTON_FOLDER_LIBRARY})
find_library(PROTON_CORE_LIBRARY qpid-proton-core HINTS ${PROTON_FOLDER_LIBRARY})
find_path(Proton_INCLUDE_DIRS proton/event.h HINTS ${PROTON_FOLDER_INCLUDE})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Proton DEFAULT_MSG
        Proton_INCLUDE_DIRS PROTON_CORE_LIBRARY)

if (NOT TARGET proton::proton)
    # Only add library if not yet added. If project referenced multiple times, the target is also created multiple times.
    add_library(proton::proton UNKNOWN IMPORTED)
    set_property(TARGET proton::proton PROPERTY IMPORTED_LOCATION "${PROTON_LIBRARY}")
    add_library(proton::proton_core UNKNOWN IMPORTED)
    set_property(TARGET proton::proton_core PROPERTY IMPORTED_LOCATION "${PROTON_CORE_LIBRARY}")
endif()

set(Proton_LIBRARIES proton::proton proton::proton_core)
set(Proton_Core_INCLUDE_DIRS ${Proton_INCLUDE_DIRS})

mark_as_advanced(Proton_INCLUDE_DIRS PROTON_LIBRARY PROTON_CORE_LIBRARY)
