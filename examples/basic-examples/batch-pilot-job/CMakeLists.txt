
set(SOURCE_FILES
        PilotJobWMS.h
        PilotJobWMS.cpp
        BatchPilotJob.cpp
        )

add_executable(wrench-batch-pilot-job-simulator ${SOURCE_FILES})

target_link_libraries(wrench-batch-pilot-job-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-batch-pilot-job-simulator  DESTINATION bin)

