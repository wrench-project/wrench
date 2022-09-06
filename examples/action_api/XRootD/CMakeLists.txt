set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(wrench-example-xrootd-basic
        EXCLUDE_FROM_ALL
        Controller.h
        Controller.cpp
        Simulator.cpp)

add_custom_target(
        wrench-example-xrootd-basic-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/xrootd_platform.xml ${CMAKE_CURRENT_BINARY_DIR}/xrootd_platform.xml ; fi ;"
        VERBATIM
)
add_dependencies(examples wrench-example-xrootd-basic)
add_dependencies(wrench-example-xrootd-basic wrench-example-xrootd-basic-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-xrootd-basic
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-xrootd-basic
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()
