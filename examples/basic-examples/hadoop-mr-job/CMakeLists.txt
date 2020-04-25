
set(SOURCE_FILES
        TwoTasksAtATimeVirtualizedClusterWMS.h
        TwoTasksAtATimeVirtualizedClusterWMS.cpp
        VirtualizedClusterBagOfTasks.cpp
        )

add_executable(wrench-example-virtualized-cluster-bag-of-tasks ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-virtualized-cluster-bag-of-tasks wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-virtualized-cluster-bag-of-tasks wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()


install(TARGETS wrench-example-virtualized-cluster-bag-of-tasks  DESTINATION bin)

