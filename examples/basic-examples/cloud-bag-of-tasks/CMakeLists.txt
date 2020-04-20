
set(SOURCE_FILES
        TwoTasksAtATimeCloudWMS.h
        TwoTasksAtATimeCloudWMS.cpp
        CloudBagOfTasks.cpp
        )

add_executable(wrench-cloud-bag-of-tasks-simulator ${SOURCE_FILES})

target_link_libraries(wrench-cloud-bag-of-tasks-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-cloud-bag-of-tasks-simulator  DESTINATION bin)

