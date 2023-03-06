
set(CMAKE_CXX_STANDARD 17)

add_custom_target(wrench_stress_test ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/wrench-stress-test)

# Add source to this project's executable.
add_executable(wrench-stress-test
        ./main.cpp
        ./Simulator.cpp
        ./Simulator.h
        ./StressTestWMS.cpp
        ./StressTestWMS.h)

add_dependencies(wrench-stress-test wrench)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wrench-stress-test
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )



if (ENABLE_BATSCHED)
    target_link_libraries(wrench-stress-test
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-stress-test
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
