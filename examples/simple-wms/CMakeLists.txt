
# source files
set(SOURCE_FILES
        optimizations/static/SimplePipelineClustering.h
        optimizations/static/SimplePipelineClustering.cpp
        optimizations/dynamic/FailureDynamicClustering.h
        optimizations/dynamic/FailureDynamicClustering.cpp
        SimpleWMS.h
        SimpleWMS.cpp
        scheduler/RandomScheduler.h
        scheduler/RandomScheduler.cpp
        scheduler/CloudScheduler.h
        scheduler/CloudScheduler.cpp
        scheduler/MinMinScheduler.h
        scheduler/MinMinScheduler.cpp
        scheduler/MaxMinScheduler.h
        scheduler/MaxMinScheduler.cpp
        scheduler/pilot_job/CriticalPathScheduler.h
        scheduler/pilot_job/CriticalPathScheduler.cpp
        )

# cloud example
set(APP_CLOUD_FILES SimpleWMSCloud.cpp)
add_executable(wrench-simple-wms-cloud ${SOURCE_FILES} ${APP_CLOUD_FILES})
target_link_libraries(wrench-simple-wms-cloud wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
install(TARGETS wrench-simple-wms-cloud DESTINATION bin)

# batch example
set(APP_BATCH_FILES SimpleWMSBatch.cpp)
add_executable(wrench-simple-wms-batch ${SOURCE_FILES} ${APP_BATCH_FILES})
target_link_libraries(wrench-simple-wms-batch wrench ${SIMGRID_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})
install(TARGETS wrench-simple-wms-batch DESTINATION bin)
