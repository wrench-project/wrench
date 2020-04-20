
set(SOURCE_FILES
        TwoTasksAtATimeWMS.h
        TwoTasksAtATimeWMS.cpp
        BareMetalBagOfTasks.cpp
        )

add_executable(wrench-bare-metal-bag-of-tasks-simulator ${SOURCE_FILES})

target_link_libraries(wrench-bare-metal-bag-of-tasks-simulator wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${LEMON_LIBRARY})

install(TARGETS wrench-bare-metal-bag-of-tasks-simulator  DESTINATION bin)

