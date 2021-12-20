# list of examples
set(EXAMPLES_CMAKEFILES_TXT
        examples/workflow_api/basic-examples/bare-metal-chain/CMakeLists.txt
        examples/workflow_api/basic-examples/bare-metal-chain-scratch/CMakeLists.txt
        examples/workflow_api/basic-examples/bare-metal-bag-of-tasks-programmatic-platform/CMakeLists.txt
        examples/workflow_api/basic-examples/bare-metal-multicore-tasks/CMakeLists.txt
        examples/workflow_api/basic-examples/bare-metal-complex-job/CMakeLists.txt
        examples/workflow_api/basic-examples/bare-metal-data-movement/CMakeLists.txt
        examples/workflow_api/basic-examples/cloud-bag-of-tasks/CMakeLists.txt
        examples/workflow_api/basic-examples/cloud-bag-of-tasks-energy/CMakeLists.txt
        examples/workflow_api/basic-examples/virtualized-cluster-bag-of-tasks/CMakeLists.txt
        examples/workflow_api/basic-examples/batch-bag-of-tasks/CMakeLists.txt
        examples/workflow_api/basic-examples/batch-pilot-job/CMakeLists.txt
        examples/workflow_api/real-workflow-example/CMakeLists.txt
        examples/workflow_api/condor-grid-example/CMakeLists.txt
        examples/workflow_api/basic-examples/io-pagecache/CMakeLists.txt

        examples/action_api/multi-action-multi-job/CMakeLists.txt
        examples/action_api/super-custom-action/CMakeLists.txt
        examples/action_api/job-action-failure/CMakeLists.txt
        examples/action_api/bare-metal-bag-of-actions/CMakeLists.txt
        examples/action_api/batch-bag-of-actions/CMakeLists.txt
        examples/action_api/cloud-bag-of-actions/CMakeLists.txt
        )

foreach (cmakefile ${EXAMPLES_CMAKEFILES_TXT})
    string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
    add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach ()

# install example files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples
        DESTINATION wrench
        FILES_MATCHING PATTERN "*.json" PATTERN "*.xml" PATTERN "*.dax"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )
