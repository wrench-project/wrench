
set(SOURCE_FILES
        WorkflowAsAsingleJobWMS.h
        WorkflowAsAsingleJobWMS.cpp
        BareMetalChainScratch.cpp
        )

add_executable(wrench-bare-metal-chain-scratch-simulator ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-bare-metal-chain-scratch-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-bare-metal-chain-scratch-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()


install(TARGETS wrench-bare-metal-chain-scratch-simulator DESTINATION bin)

