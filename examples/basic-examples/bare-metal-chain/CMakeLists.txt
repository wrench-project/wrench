
set(SOURCE_FILES
        OneTaskAtATimeWMS.h
        OneTaskAtATimeWMS.cpp
        BareMetalChain.cpp
        )

add_executable(wrench-bare-metal-chain-simulator ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-bare-metal-chain-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-bare-metal-chain-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()


install(TARGETS wrench-bare-metal-chain-simulator DESTINATION bin)

