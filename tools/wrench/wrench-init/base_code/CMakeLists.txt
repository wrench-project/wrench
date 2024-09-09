cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(my-wrench-simulator)

# Disable annoying warnings
add_definitions("-DBOOST_ALLOW_DEPRECATED_HEADERS")
#add_definitions("-DBOOST_BIND_GLOBAL_PLACEHOLDERS")

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# Find Boost
find_package(Boost REQUIRED)

# Find SimGrid
if (DEFINED SimGrid_PATH)
    if (NOT EXISTS "${SimGrid_PATH}")
        message(FATAL_ERROR "The specified SimGrid_PATH doesn't exit")
    else()
        file(GLOB LIBSIMGRID_FILES
                "${SimGrid_PATH}/lib/libsimgrid.so"
                "${SimGrid_PATH}/lib/libsimgrid.dylib"
                )
        if (NOT LIBSIMGRID_FILES)
            message(FATAL_ERROR "The specified SimGrid_PATH doesn't seem to contain a valid SimGrid installation")
        else()
            set(CMAKE_PREFIX_PATH ${SimGrid_PATH} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()
find_package(SimGrid REQUIRED)

# Find WRENCH
if (DEFINED WRENCH_PATH)
    if (NOT EXISTS "${WRENCH_PATH}")
        message(FATAL_ERROR "The specified WRENCH_PATH doesn't exit")
    else()
        file(GLOB LIBWRENCH_FILES
                "${WRENCH_PATH}/lib/libwrench.a"
                )
        if (NOT LIBWRENCH_FILES)
            message(FATAL_ERROR "The specified WRENCH_PATH doesn't seem to contain a valid WRENCH installation")
        else()
            set(CMAKE_PREFIX_PATH ${WRENCH_PATH} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()
find_package(WRENCH REQUIRED)


# include directories
include_directories(include/ /usr/local/include/ /opt/local/include/ ${WRENCH_INCLUDE_DIR} ${SimGrid_INCLUDE_DIR} ${Boost_INCLUDE_DIR})

# source files
set(SOURCE_FILES
        include/Controller.h
        src/Controller.cpp
        src/Simulator.cpp
        )

# generating the executable
add_executable(my-wrench-simulator ${SOURCE_FILES})

# Specify libraries to link
if (ENABLE_BATSCHED)
    target_link_libraries(my-wrench-simulator
            ${WRENCH_LIBRARY}
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${WRENCH_WFCOMMONS_WORKFLOW_PARSER_LIBRARY}
            -lzmq)
else()
    target_link_libraries(my-wrench-simulator
            ${WRENCH_LIBRARY}
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${WRENCH_WFCOMMONS_WORKFLOW_PARSER_LIBRARY}
            )
endif()

install(TARGETS my-wrench-simulator DESTINATION bin)
