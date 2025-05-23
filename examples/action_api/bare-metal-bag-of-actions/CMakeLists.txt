
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-bare-metal-bag-of-actions
        EXCLUDE_FROM_ALL
        ./TwoActionsAtATimeExecutionController.h
        ./TwoActionsAtATimeExecutionController.cpp
        ./BareMetalBagOfActions.cpp)

add_custom_target(
        wrench-example-bare-metal-bag-of-actions-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/two_hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-bare-metal-bag-of-actions)
add_dependencies(wrench-example-bare-metal-bag-of-actions wrench-example-bare-metal-bag-of-actions-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-bare-metal-bag-of-actions
            wrench
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-bare-metal-bag-of-actions
            wrench
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
