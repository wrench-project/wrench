# CMake find module to search for the WRENCH library.

# Copyright (c) 2022. The WRENCH Team.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

#
# USERS OF PROGRAMS USING WRENCH
# -------------------------------
#
# If cmake does not find this file, add its path to CMAKE_PREFIX_PATH:
#    CMAKE_PREFIX_PATH="/path/to/FindWRENCH.cmake:$CMAKE_PREFIX_PATH"  cmake .
#
# If this file does not find WRENCH, define WRENCH_PATH:
#    WRENCH_PATH=/path/to/wrench cmake .

#
# DEVELOPERS OF PROGRAMS USING WRENCH
# ------------------------------------
#
#  1. Include this file in your own CMakeLists.txt (before defining any target)
#     by copying it in your development tree. 
#
#  2. Afterward, if you have CMake >= 2.8.12, this will define a
#     target called 'WRENCH::WRENCH'. Use it as:
#       target_link_libraries(your-simulator WRENCH::WRENCH)
#
#    With older CMake (< 2.8.12), it simply defines several variables:
#       WRENCH_INCLUDE_DIR - the WRENCH include directories
#       WRENCH_LIBRARY - link your simulator against it to use WRENCH
#    Use as:
#      include_directories("${WRENCH_INCLUDE_DIR}" SYSTEM)
#      target_link_libraries(your-simulator ${WRENCH_LIBRARY})
#
#  Since WRENCH header files require C++17, so we set CMAKE_CXX_STANDARD to 17.
#    Change this variable in your own file if you need a later standard.

cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(WRENCH_INCLUDE_DIR
        NAMES wrench-dev.h
        PATHS ${WRENCH_PATH}/include /opt/wrench/include
        )

find_library(WRENCH_LIBRARY
        NAMES wrench
        PATHS ${WRENCH_PATH}/lib /opt/wrench/lib
        )
mark_as_advanced(WRENCH_jINCLUDE_DIR)
mark_as_advanced(WRENCH_LIBRARY)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WRENCH
        FOUND_VAR WRENCH_FOUND
        REQUIRED_VARS WRENCH_INCLUDE_DIR WRENCH_LIBRARY
        VERSION_VAR WRENCH_VERSION
        REASON_FAILURE_MESSAGE "The WRENCH package could not be located. If you installed WRENCH in a non-standard location, pass -DWRENCH_PATH=<path to location> to cmake (e.g., cmake -DWRENCH_PATH=/opt/somewhere/)"
        FAIL_MESSAGE "Could not find the WRENCH installation"
        )


if (WRENCH_FOUND AND NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    add_library(WRENCH::WRENCH SHARED IMPORTED)
    set_target_properties(WRENCH::WRENCH PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${WRENCH_INCLUDE_DIR}
            INTERFACE_COMPILE_FEATURES cxx_alias_templates
            IMPORTED_LOCATION ${WRENCH_LIBRARY}
            )
    # We need C++17, so check for it just in case the user removed it since compiling WRENCH
    if (NOT CMAKE_VERSION VERSION_LESS 3.8)
        # 3.8+ allows us to simply require C++14 (or higher)
        set_property(TARGET WRENCH::WRENCH PROPERTY INTERFACE_COMPILE_FEATURES cxx_std_17)
    elseif (NOT CMAKE_VERSION VERSION_LESS 3.1)
        # 3.1+ is similar but for certain features. We pick just one
        set_property(TARGET WRENCH::WRENCH PROPERTY INTERFACE_COMPILE_FEATURES cxx_attribute_deprecated)
    else ()
        # Old CMake can't do much. Just check the CXX_FLAGS and inform the user when a C++17 feature does not work
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
        check_cxx_source_compiles("
#if __cplusplus < 201703L
#error
#else
int main(){}
#endif
" _WRENCH_CXX17_ENABLED)
        if (NOT _WRENCH_CXX17_ENABLED)
            message(WARNING "C++17 is required to use WRENCH. Enable it with e.g. -std=c++17")
        endif ()
        unset(_WRENCH_CXX17_ENABLED CACHE)
    endif ()
endif ()

