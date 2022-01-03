
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-real-workflow
        ./SimpleWMS.h
        ./SimpleWMS.cpp
        ./SimpleWorkflowSimulator.cpp)

add_custom_target(
        wrench-example-real-workflow-files
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/cloud_batch_platform.xml ${CMAKE_CURRENT_BINARY_DIR}/cloud_batch_platform.xml
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/genome.json ${CMAKE_CURRENT_BINARY_DIR}/genome.json
        VERBATIM
)

add_dependencies(examples wrench-example-real-workflow)
add_dependencies(wrench-example-real-workflow wrench-example-real-workflow-files)
add_dependencies(wrench-example-real-workflow wrenchpegasusworkflowparser)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-real-workflow
            wrench
            wrenchpegasusworkflowparser
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-real-workflow
            wrench
            wrenchpegasusworkflowparser
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
