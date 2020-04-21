set(SOURCE_FILES
        ComplexJobWMS.h
        ComplexJobWMS.cpp
        ComplexJob.cpp
        )

add_executable(wrench-example-bare-metal-complex-job ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-bare-metal-complex-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-bare-metal-complex-job wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()

install(TARGETS wrench-example-bare-metal-complex-job DESTINATION bin)

