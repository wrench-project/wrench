# generate and install wrench-init.in tool
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/
        COMMAND /bin/sh generate.sh FindSimgrid.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init.in
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/generate.sh
        COMMENT "Generating wrench-init script"
        VERBATIM
        )

add_custom_target(wrench_init DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init)
add_dependencies(wrench wrench_init)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/wrench-init/wrench-init
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )



# install dashboard
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/wrench/dashboard
        DESTINATION wrench
        PATTERN "*"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )


# compile/install the pegasus workflow parser
set(PEGASUS_WORKFLOW_PARSER_SOURCE_FILES
        tools/pegasus/src/PegasusWorkflowParser.cpp
        )

set(PEGASUS_WORKFLOW_PARSER_HEADER_FILES
        include/wrench/tools/pegasus/PegasusWorkflowParser.h
        )

add_library(wrenchpegasusworkflowparser STATIC ${PEGASUS_WORKFLOW_PARSER_SOURCE_FILES})
install(TARGETS wrenchpegasusworkflowparser DESTINATION lib)
install(FILES "${PEGASUS_WORKFLOW_PARSER_HEADER_FILES}"
        DESTINATION include/wrench/tools/pegasus/
        )
