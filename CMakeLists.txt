cmake_minimum_required(VERSION 3.12)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(wrench CXX)

# Disable annoying warnings
add_definitions("-DBOOST_ALLOW_DEPRECATED_HEADERS")
#add_definitions("-DBOOST_BIND_GLOBAL_PLACEHOLDERS")

# Make Release the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


add_definitions("-Wall -Wswitch -Wno-unused-variable -Wno-unused-private-field -Wno-unused-local-typedefs -Wno-deprecated-builtins -Wno-unused-but-set-variable" )

if (ENABLE_BATSCHED)
    add_definitions(-DENABLE_BATSCHED)
endif ()
if (ENABLE_MESSAGE_MANAGER)
    add_definitions(-DMESSAGE_MANAGER)
endif ()
if (ENABLE_WRENCH_INTERNAL_EXCEPTIONS)
    add_definitions(-DWRENCH_INTERNAL_EXCEPTIONS)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/conf/cmake/")

# Find Boost
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
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

# Find SimGrid's FS module
if (DEFINED FSMOD_PATH)
    if (NOT EXISTS "${FSMOD_PATH}")
        message(FATAL_ERROR "The specified FSMOD_PATH doesn't exit")
    else()
        file(GLOB LIBFSMOD_FILES
                "${FSMOD_PATH}/lib/libfsmod.so"
                "${FSMOD_PATH}/lib/libfsmod.dylib"
                )
        if (NOT LIBFSMOD_FILES)
            message(FATAL_ERROR "The specified FSMOD_PATH doesn't seem to contain a valid FSMod installation")
        else()
            set(CMAKE_PREFIX_PATH ${FSMOD_PATH} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()
find_package(FSMod REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# build the version number
set(WRENCH_VERSION_MAJOR "2")
set(WRENCH_VERSION_MINOR "9")
set(WRENCH_VERSION_PATCH "0")
set(WRENCH_VERSION_EXTRA "dev")

if (${WRENCH_VERSION_PATCH} EQUAL "0")
    set(WRENCH_RELEASE_VERSION "${WRENCH_VERSION_MAJOR}.${WRENCH_VERSION_MINOR}")
else ()
    set(WRENCH_RELEASE_VERSION "${WRENCH_VERSION_MAJOR}.${WRENCH_VERSION_MINOR}.${WRENCH_VERSION_PATCH}")
endif ()

if (NOT ${WRENCH_VERSION_EXTRA} STREQUAL "")
    set(WRENCH_RELEASE_VERSION "${WRENCH_RELEASE_VERSION}-${WRENCH_VERSION_EXTRA}")
endif ()

message(STATUS "Building WRENCH Version: ${WRENCH_RELEASE_VERSION}")

include_directories(SYSTEM ${SimGrid_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${FSMOD_INCLUDE_DIR} /usr/include /usr/local/include /opt/local/include)
include_directories(src/wrench/ include/)


# For MacOS's MacPorts
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local/lib)
include_directories(/usr/local/include)

# header files
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# source files
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# test files
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
)


add_library(wrench STATIC ${SOURCE_FILES})
set_target_properties(wrench PROPERTIES VERSION ${WRENCH_RELEASE_VERSION})
target_link_libraries(wrench ${SimGrid_LIBRARY} ${FSMOD_LIBRARY})

# wrench version
add_custom_command(TARGET wrench PRE_LINK COMMAND ${CMAKE_COMMAND} -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DWRENCH_RELEASE_VERSION=${WRENCH_RELEASE_VERSION} -P ${CMAKE_HOME_DIRECTORY}/conf/cmake/Version.cmake)

install(TARGETS wrench DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# generate unit tests if Google Test is found
find_library(GTEST_LIBRARY NAMES gtest)
if(GTEST_LIBRARY)
    add_definitions(-DGTEST_USED)
    add_executable(unit_tests EXCLUDE_FROM_ALL  ${TEST_FILES})
    add_executable(unit_tests_coverage EXCLUDE_FROM_ALL  ${TEST_FILES} ${SOURCE_FILES})
    if (ENABLE_BATSCHED)
        find_library(ZMQ_LIBRARY NAMES zmq)
        target_link_libraries(unit_tests ${GTEST_LIBRARY} wrenchwfcommonsworkflowparser wrench -lpthread -lm ${ZMQ_LIBRARY})
        target_link_libraries(unit_tests_coverage ${GTEST_LIBRARY} wrenchwfcommonsworkflowparser wrench -lpthread -lm ${ZMQ_LIBRARY})
    else ()
        target_link_libraries(unit_tests ${GTEST_LIBRARY} wrenchwfcommonsworkflowparser wrench -lpthread -lm)
        target_link_libraries(unit_tests_coverage ${GTEST_LIBRARY} wrenchwfcommonsworkflowparser wrench -lpthread -lm)
    endif ()
    set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS "-g -O0")
    set_target_properties(unit_tests PROPERTIES LINK_FLAGS "")
    set_target_properties(unit_tests_coverage PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
    set_target_properties(unit_tests_coverage PROPERTIES LINK_FLAGS "--coverage")
    add_custom_command(TARGET unit_tests_coverage POST_BUILD COMMAND find . -name *.gcda -delete)
else()
    add_custom_target(unit_tests echo "ERROR: Cannot build unit_tests because Google Test (libgtest) was not found by cmake." COMMAND echo "       If you have installed Google Test, re-run cmake." VERBATIM)
endif()

# additional packages
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/DefinePackages.cmake)

# build tools
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Tools.cmake)

# build examples
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Examples.cmake)

# build benchmark
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Benchmark.cmake)

# build documentation
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Documentation.cmake)
