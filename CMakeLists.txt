# Copyright 2022-present Contributors to the logctool project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/logctool

cmake_minimum_required(VERSION 3.23)
set( project_name "logctool" )

project( "${project_name} project" )

# imath
find_package( Imath CONFIG REQUIRED )
find_package( OpenImageIO CONFIG REQUIRED )
find_package( OpenColorIO CONFIG REQUIRED )

# package
add_executable( ${project_name} "logctool.cpp" )
target_link_libraries(${project_name}
    PRIVATE
        Imath::Imath
        OpenImageIO::OpenImageIO
        OpenColorIO::OpenColorIO
)

set_property(TARGET ${project_name} PROPERTY CXX_STANDARD 14)

add_custom_command(
    TARGET ${project_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/fonts
        $<TARGET_FILE_DIR:${project_name}>/fonts
)

add_custom_command(
    TARGET ${project_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${project_name}>/resources
)
