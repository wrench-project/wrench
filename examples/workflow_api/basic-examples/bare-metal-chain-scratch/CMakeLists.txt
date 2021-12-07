
set(SOURCE_FILES
        WorkflowAsAsingleJobWMS.h
        WorkflowAsAsingleJobWMS.cpp
        BareMetalChainScratch.cpp
        )

add_executable(wrench-example-bare-metal-chain-scratch ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-bare-metal-chain-scratch wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-bare-metal-chain-scratch wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()


install(TARGETS wrench-example-bare-metal-chain-scratch DESTINATION bin)

