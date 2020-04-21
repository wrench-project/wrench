set(SOURCE_FILES
        DataMovementWMS.h
        DataMovementWMS.cpp
        DataMovement.cpp
        )

add_executable(wrench-bare-metal-data-movement-simulator ${SOURCE_FILES})

target_link_libraries(wrench-bare-metal-data-movement-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-bare-metal-data-movement-simulator DESTINATION bin)

