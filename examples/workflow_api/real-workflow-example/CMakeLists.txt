# source files
set(SOURCE_FILES
        SimpleWMS.h
        SimpleWMS.cpp
        SimpleWorkflowSimulator.cpp
        )

add_executable(wrench-example-real-workflow ${SOURCE_FILES} ${APP_BATCH_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-real-workflow wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-real-workflow wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-real-workflow DESTINATION bin)
