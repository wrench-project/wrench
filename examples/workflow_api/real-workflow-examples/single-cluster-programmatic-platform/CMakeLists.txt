
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-real-workflow-single-cluster
        EXCLUDE_FROM_ALL
        SimpleWMS.h
        SimpleWMS.cpp
        SimpleWorkflowSimulator.cpp)

add_custom_target(
        wrench-example-real-workflow-single-cluster-files
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/1000genome-chameleon-2ch-100k-001.json ${CMAKE_CURRENT_BINARY_DIR}/1000genome-chameleon-2ch-100k-001.json ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-real-workflow-single-cluster)
add_dependencies(wrench-example-real-workflow-single-cluster wrench-example-real-workflow-single-cluster-files)
add_dependencies(wrench-example-real-workflow-single-cluster wrenchwfcommonsworkflowparser)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-real-workflow-single-cluster
            wrench
            wrenchwfcommonsworkflowparser
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-real-workflow-single-cluster
            wrench
            wrenchwfcommonsworkflowparser
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
