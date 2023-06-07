# Find Imath headers and libraries.
#
# This module can take the following variables to define
# custom search locations:
#
#   ILMBASE_ROOT
#   ILMBASE_LOCATION
#
# This module defines the following variables:
#
#   IMATH_FOUND         True if Imath was found
#   IMATH_INCLUDE_DIRS  Where to find Imath header files
#   IMATH_LIBRARIES     List of Imath libraries to link against

include (FindPackageHandleStandardArgs)

find_path (IMATH_INCLUDE_DIR NAMES Imath/ImathVec.h
           HINTS ${ILMBASE_ROOT}
                 ${ILMBASE_LOCATION}
                 /usr/local/include
                 /usr/include
)

find_library (IMATH_MATH_LIBRARY NAMES Imath-3_1 Imath-3_1_d Imath
              PATH_SUFFIXES lib64 lib
              HINTS ${ILMBASE_ROOT}
                    ${ILMBASE_LOCATION}
                    /usr/local
                    /usr
)
find_library (IMATH_IEX_LIBRARY NAMES Iex-3_1 Iex-3_1_d Iex
              PATH_SUFFIXES lib64 lib
              HINTS ${ILMBASE_ROOT}
                    ${ILMBASE_LOCATION}
                    /usr/local
                    /usr
)

find_package_handle_standard_args (Imath 
    DEFAULT_MSG
    IMATH_INCLUDE_DIR
    IMATH_MATH_LIBRARY
    IMATH_IEX_LIBRARY
)

if (IMATH_FOUND)
    set (IMATH_INCLUDE_DIRS ${IMATH_INCLUDE_DIR})
    set (IMATH_LIBRARIES
        ${IMATH_MATH_LIBRARY}
        ${IMATH_IEX_LIBRARY}
    )
    message("Imath package")
    message("Include dirs: ${IMATH_INCLUDE_DIRS}")
    message("Libraries: ${IMATH_LIBRARIES}")

else ()
    set (IMATH_INCLUDE_DIRS)
    set (IMATH_LIBRARIES)
endif ()

mark_as_advanced (
    IMATH_INCLUDE_DIR
    IMATH_IEX_LIBRARY
    IMATH_MATH_LIBRARY
)
