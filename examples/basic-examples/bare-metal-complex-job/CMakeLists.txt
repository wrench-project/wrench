set(SOURCE_FILES
        ComplexJobWMS.h
        ComplexJobWMS.cpp
        ComplexJob.cpp
        )

add_executable(wrench-bare-metal-complex-job-simulator ${SOURCE_FILES})

target_link_libraries(wrench-bare-metal-complex-job-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-bare-metal-complex-job-simulator DESTINATION bin)

