# Find Imath headers and libraries.
#
# This module can take the following variables to define
# custom search locations:
#
#   OCIO_ROOT
#   OCIO_LOCATION
#
# This module defines the following variables:
#
#   OCIO_FOUND         True if OCIO was found
#   OCIO_INCLUDE_DIRS  Where to find OCIO header files
#   OCIO_LIBRARIES     List of OCIO libraries to link against

include (FindPackageHandleStandardArgs)

find_path (OCIO_INCLUDE_DIR NAMES OpenColorIO/OpenColorIO.h
           HINTS ${OCIO_ROOT}
                 ${OCIO_LOCATION}
                 /usr/local/include
                 /usr/include
)

find_library (OCIO_LIBRARY NAMES OpenColorIO.2.2
              PATH_SUFFIXES lib64 lib
              HINTS ${OCIO_ROOT}
                    ${OCIO_LOCATION}
                    /usr/local
                    /usr
)

find_package_handle_standard_args (OCIO DEFAULT_MSG
    OCIO_INCLUDE_DIR
    OCIO_LIBRARY
)

if (OCIO_FOUND)
    set (OCIO_INCLUDE_DIRS ${OCIO_INCLUDE_DIR})
    set (OCIO_LIBRARIES
        ${OCIO_LIBRARY}
    )
    message("OCIO package")
    message("Include dirs: ${OCIO_INCLUDE_DIRS}")
    message("Libraries: ${OCIO_LIBRARY}")

else ()
    set (OCIO_INCLUDE_DIRS)
    set (OCIO_LIBRARIES)
endif ()

mark_as_advanced (
    OCIO_INCLUDE_DIR
    OCIO_LIBRARY
)