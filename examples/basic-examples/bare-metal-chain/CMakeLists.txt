
set(SOURCE_FILES
        OneTaskAtATimeWMS.h
        OneTaskAtATimeWMS.cpp
        BareMetalChain.cpp
        )

add_executable(wrench-bare-metal-chain-simulator ${SOURCE_FILES})

target_link_libraries(wrench-bare-metal-chain-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-bare-metal-chain-simulator DESTINATION bin)

