
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheSingle.cpp
        )

add_executable(wrench-example-io-pagecache-single ${SOURCE_FILES})

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

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-nfs wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-nfs wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-io-pagecache-nfs DESTINATION bin)

#============================================================
set(SOURCE_FILES
        ConcurrentPipelineWMS.h
        ConcurrentPipelineWMS.cpp
        IOPageCacheNighres.cpp
        )

add_executable(wrench-example-io-pagecache-nighres ${SOURCE_FILES})

if (ENABLE_BATSCHED)
    find_library(ZMQ_LIBRARY NAMES zmq)
    target_link_libraries(wrench-example-io-pagecache-nighres wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY} ${ZMQ_LIBRARY})
else()
    target_link_libraries(wrench-example-io-pagecache-nighres wrenchwfcommonsworkflowparser wrench ${SimGrid_LIBRARY} ${PUGIXML_LIBRARY})
endif()

install(TARGETS wrench-example-io-pagecache-nighres DESTINATION bin)