
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-communicating-actions
        ./CommunicatingActionsController.h
        ./CommunicatingActionsController.cpp
        ./CommunicatingActions.cpp)

add_custom_target(
        wrench-example-communicating-actions-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/dragonfly_cluster.xml ${CMAKE_CURRENT_BINARY_DIR}/dragonfly_cluster.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-communicating-actions)
add_dependencies(wrench-example-communicating-actions wrench-example-communicating-actions-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-communicating-actions
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-communicating-actions
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
