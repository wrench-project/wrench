
set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        CondorTimestamp.h
        CondorWMS.h
        CondorWMS.cpp
        CondorGridSimulator.cpp
        ../../../tools/wfcommons/src/WfCommonsWorkflowParser.cpp
        ../../../include/wrench/tools/wfcommons/WfCommonsWorkflowParser.h
        )

# condor grid
set(CONDOR_GRID CondorGridSimulator.cpp)

# Add source to this project's executable.
add_executable(wrench-example-condor-grid-universe
        EXCLUDE_FROM_ALL
        ${SOURCE_FILES} ${CONDOR_GRID})

add_custom_target(
        wrench-example-condor-grid-universe-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/condor-grid-hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/condor-grid-hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-condor-grid-universe)
add_dependencies(wrench-example-condor-grid-universe wrench-example-condor-grid-universe-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-condor-grid-universe
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-condor-grid-universe
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()


