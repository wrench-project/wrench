
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
add_executable(wrench-example-real-workflow-cloud ${SOURCE_FILES} ${APP_CLOUD_FILES})
if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-real-workflow-cloud wrenchpegasusworkflowparser wrench ${SimGrid_qLIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-real-workflow-cloud wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-real-workflow-cloud DESTINATION bin)

# batch example
set(APP_BATCH_FILES SimpleSimulatorBatch.cpp)
add_executable(wrench-example-real-workflow-batch ${SOURCE_FILES} ${APP_BATCH_FILES})
if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-real-workflow-batch wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-real-workflow-batch wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()
install(TARGETS wrench-example-real-workflow-batch DESTINATION bin)

#add_custom_command(TARGET wrench-example-real-workflow-cloud POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_SOURCE_DIR}/examples/real-workflow-example/platform_files $<TARGET_FILE_DIR:wrench-example-real-workflow-cloud>/platform_files)
#
#add_custom_command(TARGET wrench-example-real-workflow-cloud POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_SOURCE_DIR}/examples/real-workflow-example/workflow_files $<TARGET_FILE_DIR:wrench-example-real-workflow-cloud>/workflow_files)
