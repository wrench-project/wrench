
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-serverless
        EXCLUDE_FROM_ALL
        ServerlessExampleExecutionController.h
        ServerlessExampleExecutionController.cpp
        ServerlessExample.cpp)

add_custom_target(
        wrench-example-serverless-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-serverless)
add_dependencies(wrench-example-serverless wrench-example-serverless-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-serverless
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-serverless
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
