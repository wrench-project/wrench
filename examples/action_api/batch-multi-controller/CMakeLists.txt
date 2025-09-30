
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-batch-multi-controller
        EXCLUDE_FROM_ALL
        ./BatchMultiController.cpp
        ./BatchServiceController.h
        ./BatchServiceController.cpp
        ./JobGenerationController.h
        ./JobGenerationController.cpp)

add_custom_target(
        wrench-example-batch-multi-controller-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/two_batch_services.xml ${CMAKE_CURRENT_BINARY_DIR}/two_batch_services.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-batch-multi-controller)
add_dependencies(wrench-example-batch-multi-controller wrench-example-batch-multi-controller-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-batch-multi-controller
            wrench
            ${SimGrid_LIBRARY}
	        ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-batch-multi-controller
            wrench
            ${SimGrid_LIBRARY}
	        ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
