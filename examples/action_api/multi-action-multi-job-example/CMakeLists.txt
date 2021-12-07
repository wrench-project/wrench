set(SOURCE_FILES
        MultiActionMultiJobWMS.h
        MultiActionMultiJobWMS.cpp
        MultiActionMultiJob.cpp
        )

add_executable(wrench-example-multi-action-multi-job ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-multi-action-multi-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-multi-action-multi-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-multi-action-multi-job DESTINATION bin)

