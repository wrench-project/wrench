
set(SOURCE_FILES
        TwoActionsAtATimeExecutionController.h
        TwoActionsAtATimeExecutionController.cpp
        BatchBagOfActions.cpp
        )

add_executable(wrench-example-batch-bag-of-actions ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-batch-bag-of-actions wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-batch-bag-of-actions wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-bare-metal-bag-of-actions DESTINATION bin)

