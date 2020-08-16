
set(SOURCE_FILES
        OneTaskAtATimeWMS.h
        OneTaskAtATimeWMS.cpp
        BareMetalMulticoreTasks.cpp
        )

add_executable(wrench-example-bare-metal-multicore-tasks ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-bare-metal-multicore-tasks wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-bare-metal-multicore-tasks wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-bare-metal-multicore-tasks DESTINATION bin)

