
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-bare-metal-chain-scratch
        EXCLUDE_FROM_ALL
        ./WorkflowAsAsingleJobWMS.h
        ./WorkflowAsAsingleJobWMS.cpp
        ./BareMetalChainScratch.cpp)

add_custom_target(
        wrench-example-bare-metal-chain-scratch-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts_scratch.xml ${CMAKE_CURRENT_BINARY_DIR}/two_hosts_scratch.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-bare-metal-chain-scratch)
add_dependencies(wrench-example-bare-metal-chain-scratch wrench-example-bare-metal-chain-scratch-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-bare-metal-chain-scratch
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-bare-metal-chain-scratch
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
