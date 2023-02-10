set(CMAKE_CXX_STANDARD 14)

# Add source to this project's executable.
add_executable(storage-service-proxy-basic
        EXCLUDE_FROM_ALL
        Controller.h
        Controller.cpp
        Simulator.cpp)

add_custom_target(
        storage-service-proxy-basic-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/proxy_platform.xml ${CMAKE_CURRENT_BINARY_DIR}/proxy_platform.xml ; fi ;"
        VERBATIM
)
add_dependencies(examples storage-service-proxy-basic)
add_dependencies(storage-service-proxy-basic storage-service-proxy-basic-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(storage-service-proxy-basic
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(storage-service-proxy-basic
            wrench
            ${SimGrid_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()

