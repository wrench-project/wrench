set(CMAKE_CXX_STANDARD 17)

## Compile/install the WfCommons workflow parser
set(WFCOMMONS_WORKFLOW_PARSER_SOURCE_FILES
        ./src/WfCommonsWorkflowParser.cpp
        )

set(WFCOMMONS_WORKFLOW_PARSER_HEADER_FILES
        ../../include/wrench/tools/wfcommons/WfCommonsWorkflowParser.h
        )

add_library(wrenchwfcommonsworkflowparser STATIC ${WFCOMMONS_WORKFLOW_PARSER_SOURCE_FILES})
install(TARGETS wrenchwfcommonsworkflowparser DESTINATION lib)
install(FILES "${WFCOMMONS_WORKFLOW_PARSER_HEADER_FILES}"
        DESTINATION include/wrench/tools/wfcommons/
        )

## Compile/install the WfCommons runtime subtractor
set(WFCOMMONS_WORKFLOW_COMPUTE_RUNTIME_SUBTRACTOR_SOURCE_FILES
        ./src/WfCommonsComputeRuntimeSubtractor.cpp
        )

add_executable(wrench-wfcommons-compute-runtime-subtractor ${WFCOMMONS_WORKFLOW_COMPUTE_RUNTIME_SUBTRACTOR_SOURCE_FILES})
add_dependencies(wrench-wfcommons-compute-runtime-subtractor wrench)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wrench-wfcommons-compute-runtime-subtractor
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )