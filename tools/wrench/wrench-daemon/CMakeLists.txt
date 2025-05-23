﻿set(CMAKE_CXX_STANDARD 17)

include_directories(include/ ${SimGrid_INCLUDE_DIR} ${FSMOD_INCLUDE_DIR} ${Boost_INCLUDE_DIR} /usr/include /usr/local/include /opt/local/include)

find_file(ASIO_HEADER_FOUND asio.hpp QUIET)
if (NOT ASIO_HEADER_FOUND)
	message("-- ASIO: Could not find asio.hpp (warning: the asio library is needed to compile the wrench-daemon - sudo apt install libasio-dev)")
endif()

file(GLOB WRENCH_DAEMON_CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/routes.h
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/callback-map.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_crow_routes.py ${CMAKE_CURRENT_SOURCE_DIR}/doc/wrench-openapi.json ${CMAKE_CURRENT_SOURCE_DIR}/include/routes.h ${CMAKE_CURRENT_SOURCE_DIR}/include/callback-map.h
        DEPENDS ${WRENCH_DAEMON_CPP_FILES}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/doc/wrench-openapi.json
        COMMENT "Generating CROW routes"
        VERBATIM
)

# Add needed packages
find_package(Boost REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(src/ include/ ${Boost_INCLUDE_DIR} ${SimGrid_INCLUDE_DIR} ${FSMOD_INCLUDE_DIR} /usr/local/include /opt/local/include /usr/local/include/wrench /opt/local/include)

# Add source to this project's executable.
add_executable(wrench-daemon EXCLUDE_FROM_ALL
        src/main.cpp
        src/WRENCHDaemon.cpp
        include/WRENCHDaemon.h
        src/SimulationDaemon.cpp
        include/SimulationDaemon.h
        src/SimulationLauncher.cpp
        include/SimulationLauncher.h
        include/SimulationController.h
        src/SimulationController.cpp
        include/BlockingQueue.h
        include/KeyValueStore.h
        include/REST_API.h
        include/callback-map.h
        include/routes.h
        )

add_dependencies(wrench-daemon wrench)
add_dependencies(wrench-daemon wrenchwfcommonsworkflowparser)

# Add pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-daemon
            PRIVATE Threads::Threads
            wrench
            wrenchwfcommonsworkflowparser
            ${WRENCH_PEGASUS_WORKFLOW_PARSER_LIBRARY}
            ${SimGrid_LIBRARY}
            ${FSMOD_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-daemon
            PRIVATE Threads::Threads
            wrench
            wrenchwfcommonsworkflowparser
            ${WRENCH_PEGASUS_WORKFLOW_PARSER_LIBRARY}
            ${SimGrid_LIBRARY}
            ${FSMOD_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()

install(TARGETS wrench-daemon OPTIONAL DESTINATION bin)

