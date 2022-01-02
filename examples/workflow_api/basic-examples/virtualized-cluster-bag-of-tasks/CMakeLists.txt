
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-virtualized-cluster-bag-of-tasks
        ./TwoTasksAtATimeVirtualizedClusterWMS.h
        ./TwoTasksAtATimeVirtualizedClusterWMS.cpp
        ./VirtualizedClusterBagOfTasks.cpp)

add_custom_target(
        wrench-example-virtualized-cluster-bag-of-tasks-platform-file
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts.xml
        VERBATIM
)

add_dependencies(examples wrench-example-virtualized-cluster-bag-of-tasks)
add_dependencies(wrench-example-virtualized-cluster-bag-of-tasks wrench-example-virtualized-cluster-bag-of-tasks-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-virtualized-cluster-bag-of-tasks
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-virtualized-cluster-bag-of-tasks
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
