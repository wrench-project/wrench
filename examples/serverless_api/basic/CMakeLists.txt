
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-serverless-basic
        EXCLUDE_FROM_ALL
        ServerlessExampleExecutionController.h
        ServerlessExampleExecutionController.cpp
        ServerlessExampleBasic.cpp)

add_dependencies(examples wrench-example-serverless-basic)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-serverless-basic
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-serverless-basic
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
