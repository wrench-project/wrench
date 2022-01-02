
add_custom_target(
        wrench-example-io-pagecache-files
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/single_host.xml ${CMAKE_CURRENT_BINARY_DIR}/single_host.xml
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/two_hosts.xml ${CMAKE_CURRENT_BINARY_DIR}/two_hosts.xml
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/simple_workflow.dax ${CMAKE_CURRENT_BINARY_DIR}/simple_workflow.dax
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/nighres_workflow.dax ${CMAKE_CURRENT_BINARY_DIR}/nighres_workflow.dax
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/run_multi.sh ${CMAKE_CURRENT_BINARY_DIR}/run_multi.sh
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/run_nfs.sh ${CMAKE_CURRENT_BINARY_DIR}/run_nfs.sh
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/run_nighres.sh ${CMAKE_CURRENT_BINARY_DIR}/run_nighres.sh
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/run_single.sh ${CMAKE_CURRENT_BINARY_DIR}/run_single.sh
        COMMAND /bin/cp ${CMAKE_CURRENT_SOURCE_DIR}/run_single.sh ${CMAKE_CURRENT_BINARY_DIR}/run_single.sh
        VERBATIM
)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheSingle.cpp
        )

add_executable(wrench-example-io-pagecache-single ${SOURCE_FILES})

add_dependencies(examples wrench-example-io-pagecache-single)
add_dependencies(wrench-example-io-pagecache-single wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-single wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-single wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-io-pagecache-single DESTINATION bin)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheMulti.cpp
        )

add_executable(wrench-example-io-pagecache-multi ${SOURCE_FILES})


add_dependencies(examples wrench-example-io-pagecache-multi)
add_dependencies(wrench-example-io-pagecache-multi wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-multi wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-multi wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-io-pagecache-multi DESTINATION bin)

#============================================================
set(SOURCE_FILES
        NFSPipelineWMS.h
        NFSPipelineWMS.cpp
        IOPageCacheNFS.cpp
        )

add_executable(wrench-example-io-pagecache-nfs ${SOURCE_FILES})


add_dependencies(examples wrench-example-io-pagecache-nfs)
add_dependencies(wrench-example-io-pagecache-nfs wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-nfs wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-nfs wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-io-pagecache-nfs DESTINATION bin)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheNighres.cpp
        )

add_executable(wrench-example-io-pagecache-nighres ${SOURCE_FILES})


add_dependencies(examples wrench-example-io-pagecache-nighres)
add_dependencies(wrench-example-io-pagecache-nighres wrench-example-io-pagecache-files)

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-nighres wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-nighres wrenchpegasusworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-io-pagecache-nighres DESTINATION bin)
