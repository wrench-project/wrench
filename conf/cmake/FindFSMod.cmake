# CMake find module to search for the SimGrid File System Module library.

# Copyright (c) 2024. The FSMod Team.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

#
# USERS OF PROGRAMS USING FSMod
# -------------------------------
#
# If cmake does not find this file, add its path to CMAKE_PREFIX_PATH:
#    CMAKE_PREFIX_PATH="/path/to/FindFSMod.cmake:$CMAKE_PREFIX_PATH"  cmake .
#
# If this file does not find FSMod, define FSMOD_PATH:
#    FSMOD_PATH=/path/to/fsmod cmake .

#
# DEVELOPERS OF PROGRAMS USING FSMod
# ------------------------------------
#
#  1. Include this file in your own CMakeLists.txt (before defining any target)
#     by copying it in your development tree.
#
#  2. Afterward, if you have CMake >= 2.8.12, this will define a
#     target called 'FSMOD::FSMOD'. Use it as:
#       target_link_libraries(your-simulator FSMOD::FSMOD)
#
#    With older CMake (< 2.8.12), it simply defines several variables:
#       FSMOD_INCLUDE_DIR - the FSMod include directories
#       FSMOD_LIBRARY - link your simulator against it to use FSMod
#    Use as:
#      include_directories("${FSMOD_INCLUDE_DIR}" SYSTEM)
#      target_link_libraries(your-simulator ${FSMOD_LIBRARY})
#
#  Since FSMOD header files require C++17, so we set CMAKE_CXX_STANDARD to 17.
#    Change this variable in your own file if you need a later standard.

cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(FSMOD_INCLUDE_DIR
        NAMES fsmod.hpp
        PATHS ${FSMOD_PATH}/include /opt/fsmod/include
        )

find_library(FSMOD_LIBRARY
        NAMES fsmod
        PATHS ${FSMOD_PATH}/lib /opt/fsmod/lib
        )

mark_as_advanced(FSMOD_INCLUDE_DIR)
mark_as_advanced(FSMOD_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FSMod
        FOUND_VAR FSMOD_FOUND
        REQUIRED_VARS FSMOD_INCLUDE_DIR FSMOD_LIBRARY
        VERSION_VAR FSMOD_VERSION
        REASON_FAILURE_MESSAGE "The FSMod package could not be located. If you installed FSMod in a non-standard location, pass -DFSMOD_PATH=<path to location> to cmake (e.g., cmake -DFSMOD_PATH=/opt/somewhere/)"
        FAIL_MESSAGE "Could not find the FSMod installation"
        )


if (FSMOD_FOUND AND NOT CMAKE_VERSION VERSION_LESS 2.8.12)

    add_library(FSMOD::FSMOD SHARED IMPORTED)
    set_target_properties(FSMOD::FSMOD PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${FSMOD_INCLUDE_DIR}
            INTERFACE_COMPILE_FEATURES cxx_alias_templates
            IMPORTED_LOCATION ${FSMOD_LIBRARY}
            )
    # We need C++17, so check for it just in case the user removed it since compiling FSMOD
    if (NOT CMAKE_VERSION VERSION_LESS 3.8)
        # 3.8+ allows us to simply require C++17 (or higher)
        set_property(TARGET FSMOD::FSMOD PROPERTY INTERFACE_COMPILE_FEATURES cxx_std_17)
    elseif (NOT CMAKE_VERSION VERSION_LESS 3.1)
        # 3.1+ is similar but for certain features. We pick just one
        set_property(TARGET FSMOD::FSMOD PROPERTY INTERFACE_COMPILE_FEATURES cxx_attribute_deprecated)
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
" _FSMOD_CXX17_ENABLED)
        if (NOT _FSMOD_CXX17_ENABLED)
            message(WARNING "C++17 is required to use FSMod. Enable it with e.g. -std=c++17")
        endif ()
        unset(_FSMOD_CXX14_ENABLED CACHE)
    endif ()
endif ()

