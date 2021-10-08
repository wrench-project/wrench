
set(SOURCE_FILES
        TwoTasksAtATimeWMS.h
        TwoTasksAtATimeWMS.cpp
        BareMetalBagOfTasksProgrammaticPlatform.cpp
        )

add_executable(wrench-example-bare-metal-bag-of-tasks-programmatic-platform ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-bare-metal-bag-of-tasks-programmatic-platform wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-bare-metal-bag-of-tasks-programmatic-platform wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-bare-metal-bag-of-tasks-programmatic-platform DESTINATION bin)

