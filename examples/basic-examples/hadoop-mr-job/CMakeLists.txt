
set(SOURCE_FILES
        HadoopMRJobWMS.cpp
        HadoopMRJobWMS.h
        HadoopMRJob.cpp
        )

add_executable(wrench-example-hadoop-mr-job ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-hadoop-mr-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-hadoop-mr-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()


install(TARGETS wrench-example-hadoop-mr-job  DESTINATION bin)

