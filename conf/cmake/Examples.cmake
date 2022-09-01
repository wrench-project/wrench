add_custom_target(examples)

add_subdirectory(examples/workflow_api/basic-examples/bare-metal-chain EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/bare-metal-chain-scratch EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/bare-metal-bag-of-tasks-programmatic-platform EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/bare-metal-multicore-tasks EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/bare-metal-complex-job EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/bare-metal-data-movement EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/cloud-bag-of-tasks EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/cloud-bag-of-tasks-energy EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/virtualized-cluster-bag-of-tasks EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/batch-bag-of-tasks EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/batch-pilot-job EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/real-workflow-example EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/condor-grid-example EXCLUDE_FROM_ALL)
add_subdirectory(examples/workflow_api/basic-examples/io-pagecache EXCLUDE_FROM_ALL)

add_subdirectory(examples/action_api/bare-metal-bag-of-actions EXCLUDE_FROM_ALL)
add_subdirectory(examples/action_api/batch-bag-of-actions EXCLUDE_FROM_ALL)
add_subdirectory(examples/action_api/cloud-bag-of-actions EXCLUDE_FROM_ALL)
add_subdirectory(examples/action_api/multi-action-multi-job EXCLUDE_FROM_ALL)
add_subdirectory(examples/action_api/job-action-failure EXCLUDE_FROM_ALL)
add_subdirectory(examples/action_api/super-custom-action EXCLUDE_FROM_ALL)

add_subdirectory(examples/action_api/XRootD/basic-test/ EXCLUDE_FROM_ALL)

add_custom_command(TARGET examples
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/examples/run_all_examples.sh.in | sed "s~TO_FILL_IN~${CMAKE_BINARY_DIR}/examples/~g" > ${CMAKE_BINARY_DIR}/examples/run_all_examples.sh
    COMMAND chmod +x ${CMAKE_BINARY_DIR}/examples/run_all_examples.sh
    )
