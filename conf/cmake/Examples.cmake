# list of examples
set(EXAMPLES_CMAKEFILES_TXT
        examples/simple-example/CMakeLists.txt
        examples/basic-examples/bare-metal-chain/CMakeLists.txt
        examples/basic-examples/bare-metal-chain-scratch/CMakeLists.txt
        examples/basic-examples/bare-metal-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/bare-metal-complex-job/CMakeLists.txt
        examples/basic-examples/cloud-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/virtualized-cluster-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/batch-bag-of-tasks/CMakeLists.txt
        examples/basic-examples/batch-pilot-job/CMakeLists.txt
        )

foreach (cmakefile ${EXAMPLES_CMAKEFILES_TXT})
    string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
    add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach ()
