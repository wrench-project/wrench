
set(SOURCE_FILES
        PilotJobWMS.h
        PilotJobWMS.cpp
        BatchPilotJob.cpp
        )

add_executable(wrench-example-batch-pilot-job ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-batch-pilot-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-batch-pilot-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-batch-pilot-job  DESTINATION bin)

