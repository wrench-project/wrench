
set(SOURCE_FILES
        WorkflowAsAsingleJobWMS.h
        WorkflowAsAsingleJobWMS.cpp
        BareMetalChainScratch.cpp
        )

add_executable(wrench-bare-metal-chain-scratch-simulator ${SOURCE_FILES})

target_link_libraries(wrench-bare-metal-chain-scratch-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-bare-metal-chain-scratch-simulator DESTINATION bin)

