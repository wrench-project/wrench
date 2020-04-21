
set(SOURCE_FILES
        TwoTasksAtATimeVirtualizedClusterWMS.h
        TwoTasksAtATimeVirtualizedClusterWMS.cpp
        VirtualizedClusterBagOfTasks.cpp
        )

add_executable(wrench-virtualized-cluster-bag-of-tasks-simulator ${SOURCE_FILES})

target_link_libraries(wrench-virtualized-cluster-bag-of-tasks-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-virtualized-cluster-bag-of-tasks-simulator  DESTINATION bin)

