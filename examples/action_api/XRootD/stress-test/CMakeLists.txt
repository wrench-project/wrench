set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-xrootd-stress
        EXCLUDE_FROM_ALL
        ./include/Controller.h
        ./src/Controller.cpp 
        ./include/platformCreator.h
        ./src/platformCreator.cpp
        ./src/Simulator.cpp)


add_dependencies(examples wrench-example-xrootd-stress)

if (ENABLE_BATSCHED)
    target_link_libraries(wwrench-example-xrootd-stress
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-xrootd-stress
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
