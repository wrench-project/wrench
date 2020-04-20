
set(SOURCE_FILES
        TwoTasksAtATimeBatchWMS.h
        TwoTasksAtATimeBatchWMS.cpp
        BatchBagOfTasks.cpp
        )

add_executable(wrench-batch-bag-of-tasks-simulator ${SOURCE_FILES})

target_link_libraries(wrench-batch-bag-of-tasks-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-batch-bag-of-tasks-simulator  DESTINATION bin)

