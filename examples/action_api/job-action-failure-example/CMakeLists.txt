set(SOURCE_FILES
        JobActionFailureController.h
        JobActionFailureController.cpp
        JobActionFailure.cpp
        )

add_executable(wrench-example-job-action-failure ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-job-action-failure wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-job-action-failure wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-job-action-failure DESTINATION bin)

