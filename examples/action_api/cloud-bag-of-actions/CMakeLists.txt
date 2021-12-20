
set(SOURCE_FILES
        GreedyExecutionController.h
        GreedyExecutionController.cpp
        CloudBagOfActions.cpp
        )

add_executable(wrench-example-cloud-bag-of-actions ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-cloud-bag-of-actions wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-cloud-bag-of-actions wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-bare-metal-bag-of-actions DESTINATION bin)

