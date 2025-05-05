set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
add_executable(wrench-example-storage-service-proxy-basic
        EXCLUDE_FROM_ALL
        Controller.h
        Controller.cpp
        Simulator.cpp)

add_custom_target(
        wrench-example-storage-service-proxy-basic-platform-file
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/proxy_platform.xml ${CMAKE_CURRENT_BINARY_DIR}/proxy_platform.xml ; fi ;"
        VERBATIM
)
add_dependencies(examples wrench-example-storage-service-proxy-basic)
add_dependencies(wrench-example-storage-service-proxy-basic wrench-example-storage-service-proxy-basic-platform-file)

if (ENABLE_BATSCHED)
    target_link_libraries(wrench-example-storage-service-proxy-basic
            wrench
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            ${ZMQ_LIBRARY}
            )
else()
    target_link_libraries(wrench-example-storage-service-proxy-basic
            wrench
            ${SimGrid_LIBRARY}
	    ${FSMOD_LIBRARY}
            ${Boost_LIBRARIES}
            )
endif()

