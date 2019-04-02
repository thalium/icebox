# Copyright 2016-2018 by Martin Moene
#
# https://github.com/martinmoene/expected-lite
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required( VERSION 3.5 )

# expected-lite version, updated by script/update-version.py:

set( expected_lite_version "0.2.0" )

# At default, enable building and performing of tests and building of examples:

option( EXPECTED_LITE_OPT_BUILD_TESTS    "Build and perform expected-lite tests" OFF )
option( EXPECTED_LITE_OPT_BUILD_EXAMPLES "Build expected-lite examples" OFF )
set(    EXPEXTED_P0323R  "99" STRING     "Specify proposal revision compatibility (99: latest)" )

option( EXPECTED_LITE_OPT_SELECT_STD     "Select std::expected"    OFF )
option( EXPECTED_LITE_OPT_SELECT_NONSTD  "Select nonstd::expected" OFF )

include( GNUInstallDirs )

project( expected_lite LANGUAGES CXX )

set( package_name "expected-lite" )

set( include_source_dir "${PROJECT_SOURCE_DIR}/include" )

# Interface library:

add_library(
    ${package_name} INTERFACE )

target_include_directories(
    ${package_name} INTERFACE "$<BUILD_INTERFACE:${include_source_dir}>" )

# Installation:

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )

# If requested, build and perform tests, build examples:

enable_testing()

if ( EXPECTED_LITE_OPT_BUILD_TESTS )
    add_subdirectory( test )
endif()

if ( EXPECTED_LITE_OPT_BUILD_EXAMPLES )
    add_subdirectory( example )
endif()

# end of file
