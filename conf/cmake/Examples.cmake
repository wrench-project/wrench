# list of examples
set(EXAMPLES_CMAKEFILES_TXT
        examples/basic-examples/bare-metal-chain/CMakeLists.txt
        examples/basic-examples/bare-metal-chain-scratch/CMakeLists.txt
        examples/basic-examples/bare-metal-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/bare-metal-multicore-tasks/CMakeLists.txt
        examples/basic-examples/bare-metal-complex-job/CMakeLists.txt
        examples/basic-examples/bare-metal-data-movement/CMakeLists.txt
        examples/basic-examples/cloud-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/cloud-bag-of-tasks-energy/CMakeLists.txt
        examples/basic-examples/virtualized-cluster-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/batch-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/batch-pilot-job/CMakeLists.txt
        examples/real-workflow-example/CMakeLists.txt
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
