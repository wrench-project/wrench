
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-batch-pilot-job
        ./PilotJobWMS.h
        ./PilotJobWMS.cpp
        ./BatchPilotJob.cpp)

add_custom_target(
        wrench-example-batch-pilot-job-platform-file
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_scratch.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts_scratch.xml
        VERBATIM
)

add_dependencies(examples wrench-example-batch-pilot-job)
add_dependencies(wrench-example-batch-pilot-job wrench-example-batch-pilot-job-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-batch-pilot-job
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-batch-pilot-job
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
