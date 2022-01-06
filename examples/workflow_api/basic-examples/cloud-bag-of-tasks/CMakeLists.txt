
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-cloud-bag-of-tasks
        EXCLUDE_FROM_ALL
        ./TwoTasksAtATimeCloudWMS.h
        ./TwoTasksAtATimeCloudWMS.cpp
        ./CloudBagOfTasks.cpp)

add_custom_target(
        wrench-example-cloud-bag-of-tasks-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-cloud-bag-of-tasks)
add_dependencies(wrench-example-cloud-bag-of-tasks wrench-example-cloud-bag-of-tasks-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-cloud-bag-of-tasks
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-cloud-bag-of-tasks
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
