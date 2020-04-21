
set(SOURCE_FILES
        TwoTasksAtATimeWMS.h
        TwoTasksAtATimeWMS.cpp
        BareMetalBagOfTasks.cpp
        )

add_executable(wrench-bare-metal-bag-of-tasks-simulator ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-bare-metal-bag-of-tasks-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-bare-metal-bag-of-tasks-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()

install(TARGETS wrench-bare-metal-bag-of-tasks-simulator  DESTINATION bin)

