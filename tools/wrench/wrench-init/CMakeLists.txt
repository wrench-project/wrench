add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/wrench-init
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND /bin/sh generate.sh ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wrench-init.in
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate.sh
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/CMakeLists.txt
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/platform.xml
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/Simulator_ACTION.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/Controller_ACTION.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/Controller_ACTION.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/Simulator_WORKFLOW.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/Controller_WORKFLOW.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base_code/Controller_WORKFLOW.h
        COMMENT "Generating wrench-init script"
        VERBATIM
)

add_custom_target(wrench_init ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wrench-init)
add_dependencies(wrench_init wrench)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wrench-init
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )

