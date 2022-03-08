
set(CMAKE_CXX_STANDARD 14)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/REST_API_generated_code.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/doc/
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_REST_API.py ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/doc/index.html
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        COMMENT "Generating REST API code and documentation"
        VERBATIM
)


# Add needed packages
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(src/ include/ ${Boost_INCLUDE_DIR} ${SimGrid_INCLUDE_DIR}/include /usr/local/include /opt/local/include /usr/local/include/wrench)

# Add source to this project's executable.
add_executable(wrench-daemon
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
       )

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
