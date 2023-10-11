
add_custom_target(
        wrench-example-io-pagecache-files
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/single_host.xml ${CMAKE_CURRENT_BINARY_DIR}/single_host.xml ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/two_hosts.xml ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/simple_workflow.json ${CMAKE_CURRENT_BINARY_DIR}/simple_workflow.json ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/nighres_workflow.json ${CMAKE_CURRENT_BINARY_DIR}/nighres_workflow.json ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/run_multi.sh ${CMAKE_CURRENT_BINARY_DIR}/run_multi.sh ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/run_nfs.sh ${CMAKE_CURRENT_BINARY_DIR}/run_nfs.sh ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/run_nighres.sh ${CMAKE_CURRENT_BINARY_DIR}/run_nighres.sh ; fi ;"
        COMMAND /bin/sh -c "if [ '${CMAKE_CURRENT_SOURCE_DIR}' != '${CMAKE_CURRENT_BINARY_DIR}' ]; then /bin/cp -f ${CMAKE_CURRENT_SOURCE_DIR}/run_single.sh ${CMAKE_CURRENT_BINARY_DIR}/run_single.sh ; fi ;"
        VERBATIM
)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheSingle.cpp
        )

add_executable(wrench-example-io-pagecache-single
        EXCLUDE_FROM_ALL
        ${SOURCE_FILES})

add_dependencies(examples wrench-example-io-pagecache-single)
add_dependencies(wrench-example-io-pagecache-single wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-single wrench ${SimGrid_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-single wrench ${SimGrid_LIBRARY})
endif()

#install(TARGETS wrench-example-io-pagecache-single DESTINATION bin)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheMulti.cpp
        )

add_executable(wrench-example-io-pagecache-multi
        EXCLUDE_FROM_ALL
        ${SOURCE_FILES})


add_dependencies(examples wrench-example-io-pagecache-multi)
add_dependencies(wrench-example-io-pagecache-multi wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-multi wrench ${SimGrid_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-multi wrench ${SimGrid_LIBRARY})
endif()

#install(TARGETS wrench-example-io-pagecache-multi DESTINATION bin)

#============================================================
set(SOURCE_FILES
        NFSPipelineWMS.h
        NFSPipelineWMS.cpp
        IOPageCacheNFS.cpp
        )

add_executable(wrench-example-io-pagecache-nfs
        EXCLUDE_FROM_ALL
        ${SOURCE_FILES})


add_dependencies(examples wrench-example-io-pagecache-nfs)
add_dependencies(wrench-example-io-pagecache-nfs wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-nfs wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-nfs wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY})
endif()

#install(TARGETS wrench-example-io-pagecache-nfs DESTINATION bin)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheNighres.cpp
        )

add_executable(wrench-example-io-pagecache-nighres
        EXCLUDE_FROM_ALL
        ${SOURCE_FILES})


add_dependencies(examples wrench-example-io-pagecache-nighres)
add_dependencies(wrench-example-io-pagecache-nighres wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-nighres wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-nighres wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY})
endif()

#install(TARGETS wrench-example-io-pagecache-nighres DESTINATION bin)
