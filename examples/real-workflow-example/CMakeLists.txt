
# source files
set(SOURCE_FILES
        optimizations/static/SimplePipelineClustering.h
        optimizations/static/SimplePipelineClustering.cpp
        optimizations/dynamic/FailureDynamicClustering.h
        optimizations/dynamic/FailureDynamicClustering.cpp
        SimpleWMS.h
        SimpleWMS.cpp
        scheduler/CloudStandardJobScheduler.h
        scheduler/CloudStandardJobScheduler.cpp
        scheduler/BatchStandardJobScheduler.cpp
        scheduler/BatchStandardJobScheduler.h
        scheduler/pilot_job/CriticalPathPilotJobScheduler.h
        scheduler/pilot_job/CriticalPathPilotJobScheduler.cpp
        )

# cloud example
set(APP_CLOUD_FILES SimpleSimulatorCloud.cpp)
add_executable(wrench-real-workflow-example-cloud ${SOURCE_FILES} ${APP_CLOUD_FILES})
if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-real-workflow-example-cloud wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-real-workflow-example-cloud wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()

install(TARGETS wrench-real-workflow-example-cloud DESTINATION bin)

# batch example
set(APP_BATCH_FILES SimpleSimulatorBatch.cpp)
add_executable(wrench-real-workflow-example-batch ${SOURCE_FILES} ${APP_BATCH_FILES})
if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-real-workflow-example-batch wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-real-workflow-example-batch wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()
install(TARGETS wrench-real-workflow-example-batch DESTINATION bin)

add_custom_command(TARGET wrench-real-workflow-example-cloud POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/examples/real-workflow-example/platform_files $<TARGET_FILE_DIR:wrench-real-workflow-example-cloud>/platform_files)

add_custom_command(TARGET wrench-real-workflow-example-cloud POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/examples/real-workflow-example/workflow_files $<TARGET_FILE_DIR:wrench-real-workflow-example-cloud>/workflow_files)
