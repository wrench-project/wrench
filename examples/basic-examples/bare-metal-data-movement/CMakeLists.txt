set(SOURCE_FILES
        DataMovementWMS.h
        DataMovementWMS.cpp
        DataMovement.cpp
        )

add_executable(wrench-example-bare-metal-data-movement ${SOURCE_FILES})

target_link_libraries(wrench-example-bare-metal-data-movement wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-bare-metal-data-movement wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-bare-metal-data-movement wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-bare-metal-data-movement DESTINATION bin)

