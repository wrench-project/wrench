
set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-super-custom-action
        ./SuperCustomActionController.h
        ./SuperCustomActionController.cpp
        ./SuperCustomAction.cpp)

add_custom_target(
        wrench-example-super-custom-action-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-super-custom-action)
add_dependencies(wrench-example-super-custom-action wrench-example-super-custom-action-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-super-custom-action
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-super-custom-action
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
