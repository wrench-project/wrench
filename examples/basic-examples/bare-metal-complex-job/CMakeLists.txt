set(SOURCE_FILES
        ComplexJobWMS.h
        ComplexJobWMS.cpp
        ComplexJob.cpp
        )

add_executable(wrench-bare-metal-complex-job-simulator ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-bare-metal-complex-job-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-bare-metal-complex-job-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()

install(TARGETS wrench-bare-metal-complex-job-simulator DESTINATION bin)

