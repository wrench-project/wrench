# Compile/install the wfcommons tools
add_subdirectory(${CMAKE_HOME_DIRECTORY}/tools/wfcommons)

# Compile/install wrench-init
add_subdirectory("${CMAKE_HOME_DIRECTORY}/tools/wrench/wrench-init")

# Compile/install wrench-daemon
add_subdirectory("${CMAKE_HOME_DIRECTORY}/tools/wrench/wrench-daemon" EXCLUDE_FROM_ALL)

