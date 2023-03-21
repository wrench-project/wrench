
set(CMAKE_CXX_STANDARD 14)

# ToDo: change it to my script generate html form swagger gen maybe?
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/REST_API_generated_code.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/doc/
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_REST_API.py ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/doc/index.html
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        COMMENT "Generating REST API code and documentation"
        VERBATIM
)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/routes.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/doc/
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_crow_routes.py ${CMAKE_CURRENT_SOURCE_DIR}/doc/wrench-openapi.json ${CMAKE_CURRENT_SOURCE_DIR}/include/routes.h
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
        include/REST_API_generated_code.h
        include/routes.h)

# Add pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-daemon
            PRIVATE Threads::Threads
            wrench
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
            ${WRENCH_PEGASUS_WORKFLOW_PARSER_LIBRARY}
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()

install(TARGETS wrench-daemon DESTINATION bin)
