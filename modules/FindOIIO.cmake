# Find Imath headers and libraries.
#
# This module can take the following variables to define
# custom search locations:
#
#   OIIO_ROOT
#   OIIO_LOCATION
#
# This module defines the following variables:
#
#   OIIO_FOUND         True if OIIO was found
#   OIIO_INCLUDE_DIRS  Where to find OIIO header files
#   OIIO_LIBRARIES     List of OIIO libraries to link against

include (FindPackageHandleStandardArgs)

find_path (OIIO_INCLUDE_DIR NAMES OpenImageIO/oiioversion.h
           HINTS ${OIIO_ROOT}
                 ${OIIO_LOCATION}
                 /usr/local/include
                 /usr/include
)

find_library (OIIO_LIBRARY NAMES OpenImageIO OpenImageIO_d
              PATH_SUFFIXES lib64 lib
              HINTS ${OIIO_ROOT}
                    ${OIIO_LOCATION}
                    /usr/local
                    /usr
)

find_library (OIIO_UTIL_LIBRARY NAMES OpenImageIO_Util OpenImageIO_Util_d
              PATH_SUFFIXES lib64 lib
              HINTS ${OIIO_ROOT}
                    ${OIIO_LOCATION}
                    /usr/local
                    /usr
)

find_package_handle_standard_args (OIIO DEFAULT_MSG
    OIIO_INCLUDE_DIR
    OIIO_LIBRARY
    OIIO_UTIL_LIBRARY
)

if (OIIO_FOUND)
    set (OIIO_INCLUDE_DIRS ${OIIO_INCLUDE_DIR})
    set (OIIO_LIBRARIES
        ${OIIO_LIBRARY}
        ${OIIO_UTIL_LIBRARY}
    )
    message("OIIO package")
    message("Include dirs: ${OIIO_INCLUDE_DIRS}")
    message("Libraries: ${OIIO_LIBRARIES}")

else ()
    set (OIIO_INCLUDE_DIRS)
    set (OIIO_LIBRARIES)
endif ()

mark_as_advanced (
    OIIO_INCLUDE_DIR
    OIIO_LIBRARY
)
