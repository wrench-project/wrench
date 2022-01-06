
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-job-action-failure
        EXCLUDE_FROM_ALL
        ./JobActionFailureController.h
        ./JobActionFailureController.cpp
        ./JobActionFailure.cpp)

add_custom_target(
        wrench-example-job-action-failure-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-job-action-failure)
add_dependencies(wrench-example-job-action-failure wrench-example-job-action-failure-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-job-action-failure
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-job-action-failure
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
