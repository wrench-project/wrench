
# source files
set(SOURCE_FILES
        optimizations/static/SimplePipelineClustering.h
        optimizations/static/SimplePipelineClustering.cpp
        optimizations/dynamic/FailureDynamicClustering.h
        optimizations/dynamic/FailureDynamicClustering.cpp
        SimpleWMS.h
        SimpleWMS.cpp
        scheduler/MinMinScheduler.h
        scheduler/MinMinScheduler.cpp
        scheduler/MaxMinScheduler.h
        scheduler/MaxMinScheduler.cpp
        scheduler/pilot_job/CriticalPathScheduler.h
        scheduler/pilot_job/CriticalPathScheduler.cpp
        main.cpp)

add_executable(simple-wms ${SOURCE_FILES})
target_link_libraries(simple-wms wrench ${SIMGRID_LIB} ${LEMON_LIB} ${PUGIXML_LIB})
install(TARGETS simple-wms DESTINATION bin)
