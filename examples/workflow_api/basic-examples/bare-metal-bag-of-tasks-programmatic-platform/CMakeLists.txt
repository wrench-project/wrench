
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-bare-metal-bag-of-tasks-programmatic-platform
        ./TwoTasksAtATimeWMS.h
        ./TwoTasksAtATimeWMS.cpp
        ./BareMetalBagOfTasksProgrammaticPlatform.cpp)

add_custom_target(
        wrench-example-bare-metal-bag-of-tasks-programmatic-platform-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/two_hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-bare-metal-bag-of-tasks-programmatic-platform)
add_dependencies(wrench-example-bare-metal-bag-of-tasks-programmatic-platform wrench-example-bare-metal-bag-of-tasks-programmatic-platform-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-bare-metal-bag-of-tasks-programmatic-platform
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-bare-metal-bag-of-tasks-programmatic-platform
            wrench
            ${SimGrid_LIBRARY}
            ${PUGIXML_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
