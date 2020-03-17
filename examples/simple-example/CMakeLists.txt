
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
add_executable(wrench-simple-example-cloud ${SOURCE_FILES} ${APP_CLOUD_FILES})

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-simple-example-cloud wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} -lzmq)
else()
    target_link_libraries(wrench-simple-example-cloud wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()



install(TARGETS wrench-simple-example-cloud DESTINATION bin)

# batch example
set(APP_BATCH_FILES SimpleSimulatorBatch.cpp)
add_executable(wrench-simple-example-batch ${SOURCE_FILES} ${APP_BATCH_FILES})
if (ENABLE_BATSCHED)
    target_link_libraries(wrench-simple-example-batch wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} -lzmq)
else()
    target_link_libraries(wrench-simple-example-batch wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()
install(TARGETS wrench-simple-example-batch DESTINATION bin)

add_custom_command(TARGET wrench-simple-example-cloud POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/examples/simple-example/platform_files $<TARGET_FILE_DIR:wrench-simple-example-cloud>/platform_files)

add_custom_command(TARGET wrench-simple-example-cloud POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/examples/simple-example/workflow_files $<TARGET_FILE_DIR:wrench-simple-example-cloud>/workflow_files)
