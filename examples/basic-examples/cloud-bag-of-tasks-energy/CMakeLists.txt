
set(SOURCE_FILES
        TwoTasksAtATimeCloudWMS.h
        TwoTasksAtATimeCloudWMS.cpp
        CloudBagOfTasksEnergy.cpp
        )

add_executable(wrench-example-cloud-bag-of-tasks-energy ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-cloud-bag-of-tasks-energy wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-cloud-bag-of-tasks-energy wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-cloud-bag-of-tasks-energy  DESTINATION bin)

