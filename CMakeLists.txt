# Copyright 2022-present Contributors to the colorpicker project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/logctool

cmake_minimum_required(VERSION 3.23)
set( project_name "logctool" )

project( "${project_name} project" )

# modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )

# imath
find_package( Imath REQUIRED )
find_package( OIIO REQUIRED )
find_package( OCIO REQUIRED )

# package
add_executable( ${project_name} "logctool.cpp" )
set_property(TARGET ${project_name} PROPERTY CXX_STANDARD 14)

include_directories (
    ${IMATH_INCLUDE_DIRS}
    ${OIIO_INCLUDE_DIRS}
    ${OCIO_INCLUDE_DIR}
)

target_link_libraries( ${project_name}
    ${IMATH_LIBRARIES}  
    ${OIIO_LIBRARIES}
    ${OCIO_LIBRARIES}
)
