
set(SOURCE_FILES
        PilotJobWMS.h
        PilotJobWMS.cpp
        BatchPilotJob.cpp
        )

add_executable(wrench-batch-pilot-job-simulator ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-batch-pilot-job-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-batch-pilot-job-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()

install(TARGETS wrench-batch-pilot-job-simulator  DESTINATION bin)

