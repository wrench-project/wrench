
# source files
set(SOURCE_FILES
        optimizations/static/SimplePipelineClustering.h
        optimizations/static/SimplePipelineClustering.cpp
        optimizations/dynamic/FailureDynamicClustering.h
        optimizations/dynamic/FailureDynamicClustering.cpp
        SimpleWMS.h
        SimpleWMS.cpp
        scheduler/RandomStandardJobScheduler.h
        scheduler/RandomStandardJobScheduler.cpp
        scheduler/CloudStandardJobScheduler.h
        scheduler/CloudStandardJobScheduler.cpp
        scheduler/MinMinStandardJobScheduler.h
        scheduler/MinMinStandardJobScheduler.cpp
        scheduler/MaxMinStandardJobScheduler.h
        scheduler/MaxMinStandardJobScheduler.cpp
        scheduler/BatchStandardJobScheduler.cpp
        scheduler/BatchStandardJobScheduler.h
        scheduler/pilot_job/CriticalPathPilotJobScheduler.h
        scheduler/pilot_job/CriticalPathPilotJobScheduler.cpp
        )

# cloud example
set(APP_CLOUD_FILES SimpleWMSCloud.cpp)
add_executable(wrench-simple-wms-cloud ${SOURCE_FILES} ${APP_CLOUD_FILES})
if (ENABLE_BATSCHED)
    target_link_libraries(wrench-simple-wms-cloud wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} -lzmq)
else()
    target_link_libraries(wrench-simple-wms-cloud wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()
install(TARGETS wrench-simple-wms-cloud DESTINATION bin)

# batch example
set(APP_BATCH_FILES SimpleWMSBatch.cpp)
add_executable(wrench-simple-wms-batch ${SOURCE_FILES} ${APP_BATCH_FILES})
if (ENABLE_BATSCHED)
    target_link_libraries(wrench-simple-wms-batch wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY} -lzmq)
else()
    target_link_libraries(wrench-simple-wms-batch wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
endif()
install(TARGETS wrench-simple-wms-batch DESTINATION bin)
