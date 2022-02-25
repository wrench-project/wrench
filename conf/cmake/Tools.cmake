add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/
        COMMAND /bin/sh generate.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init.in
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/generate.sh
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/CMakeLists.txt
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/platform.xml
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/Simulator_ACTION.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/Controller_ACTION.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/Controller_ACTION.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/Simulator_WORKFLOW.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/Controller_WORKFLOW.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/base_code/Controller_WORKFLOW.h
        COMMENT "Generating wrench-init script"
        VERBATIM
)

add_custom_target(wrench_init ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init)
add_dependencies(wrench_init wrench)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tools/wrench/wrench-init/wrench-init
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )

# install wrench-dashboard
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tools/wrench/wrench-dashboard/wrench-dashboard
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )

configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-dashboard/wrench-dashboard.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tools/wrench/wrench-dashboard/wrench-dashboard"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tools/wrench/wrench-dashboard/wrench-dashboard
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )

# compile/install the wfcommons workflow parser
set(WFCOMMONS_WORKFLOW_PARSER_SOURCE_FILES
        tools/wfcommons/src/WfCommonsWorkflowParser.cpp
        )

set(WFCOMMONS_WORKFLOW_PARSER_HEADER_FILES
        include/wrench/tools/wfcommons/WfCommonsWorkflowParser.h
        )

add_library(wrenchwfcommonsworkflowparser STATIC ${WFCOMMONS_WORKFLOW_PARSER_SOURCE_FILES})
install(TARGETS wrenchwfcommonsworkflowparser DESTINATION lib)
install(FILES "${WFCOMMONS_WORKFLOW_PARSER_HEADER_FILES}"
        DESTINATION include/wrench/tools/wfcommons/
        )

