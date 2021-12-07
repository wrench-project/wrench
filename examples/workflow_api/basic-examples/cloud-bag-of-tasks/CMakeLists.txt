
set(SOURCE_FILES
        TwoTasksAtATimeCloudWMS.h
        TwoTasksAtATimeCloudWMS.cpp
        CloudBagOfTasks.cpp
        )

add_executable(wrench-example-cloud-bag-of-tasks ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-cloud-bag-of-tasks wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-cloud-bag-of-tasks wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-cloud-bag-of-tasks  DESTINATION bin)

