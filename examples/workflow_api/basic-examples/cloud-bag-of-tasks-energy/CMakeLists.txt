
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-cloud-bag-of-tasks-energy
        EXCLUDE_FROM_ALL
        ./TwoTasksAtATimeCloudWMS.h
        ./TwoTasksAtATimeCloudWMS.cpp
        ./CloudBagOfTasksEnergy.cpp)

add_custom_target(
        wrench-example-cloud-bag-of-tasks-energy-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/four_hosts_energy.xml ${CMAKE_CURRENT_BINARY_DIR}/four_hosts_energy.xml ; fi ;"
        VERBATIM
)

add_dependencies(examples wrench-example-cloud-bag-of-tasks-energy)
add_dependencies(wrench-example-cloud-bag-of-tasks-energy wrench-example-cloud-bag-of-tasks-energy-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-cloud-bag-of-tasks-energy
            wrench
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-cloud-bag-of-tasks-energy
            wrench
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
