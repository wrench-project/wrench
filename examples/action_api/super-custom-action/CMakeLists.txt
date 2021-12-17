set(SOURCE_FILES
        SuperCustomActionController.h
        SuperCustomActionController.cpp
        SuperCustomAction.cpp
        )

add_executable(wrench-example-super-custom-action ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-super-custom-action wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-super-custom-action wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-super-custom-action DESTINATION bin)

