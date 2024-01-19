
set(CMAKE_CXX_STANDARD 17)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/routes.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_crow_routes.py ${CMAKE_CURRENT_SOURCE_DIR}/doc/wrench-openapi.json ${CMAKE_CURRENT_SOURCE_DIR}/include/routes.h ${CMAKE_CURRENT_SOURCE_DIR}/include/callback-map.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/doc/wrench-openapi.json
        COMMENT "Generating CROW routes"
        VERBATIM
)


# Add needed packages
find_package(Boost REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(src/ include/ ${Boost_INCLUDE_DIR} ${SimGrid_INCLUDE_DIR}/include /usr/local/include /opt/local/include /usr/local/include/wrench /opt/local/include)


# Add source to this project's executable.
add_executable(wrench-daemon EXCLUDE_FROM_ALL
        include/httplib.h
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
        include/routes.h)

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
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()

install(TARGETS wrench-daemon OPTIONAL DESTINATION bin)

