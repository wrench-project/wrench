# source files
set(SOURCE_FILES
        CondorTimestamp.h
        CondorWMS.h
        CondorWMS.cpp
        CondorGridSimulator.cpp
        ../../tools/wfcommons/src/WfCommonsWorkflowParser.cpp
        ../../include/wrench/tools/wfcommons/WfCommonsWorkflowParser.h
        )

# condor grid
set(CONDOR_GRID CondorGridSimulator.cpp)
add_executable(wrench-example-condor-grid-universe ${SOURCE_FILES} ${CONDOR_GRID})
target_link_libraries(wrench-example-condor-grid-universe wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY} )

install(TARGETS wrench-example-condor-grid-universe DESTINATION bin)

