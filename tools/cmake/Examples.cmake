# list of examples
set(EXAMPLES_CMAKEFILES_TXT
        examples/simple-example/CMakeLists.txt
        )


foreach (cmakefile ${EXAMPLES_CMAKEFILES_TXT})
    string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
    add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach ()
