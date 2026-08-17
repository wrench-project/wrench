/**
* Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVERLESSCOMPUTESERVICEPROPERTY_H
#define WRENCH_SERVERLESSCOMPUTESERVICEPROPERTY_H

#include <map>

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
    * @brief Configurable properties for a ServerlessComputeService
    */
    class ServerlessComputeServiceProperty : public ComputeServiceProperty {

    public:
        /** @brief The overhead for the compute service to process an incoming request (default value: "0", default unit: seconds):
         *         Examples: "5", "5s", "5000ms", etc.
         **/
        DECLARE_PROPERTY_NAME(INVOCATION_PROCESSING_OVERHEAD);

        /** @brief The overhead to start a container after the image has been loaded into RAM (default value: "0", default unit: seconds):
         *         Examples: "5", "5s", "5000ms", etc.
         **/
        DECLARE_PROPERTY_NAME(CONTAINER_STARTUP_OVERHEAD);

        /** @brief The timeout before an idle container is terminated and its disk/resources reclaimed.
         **/
        DECLARE_PROPERTY_NAME(CONTAINER_IDLE_TIMEOUT);

        /** @brief The policy used to evict idle containers if space is needed at a compute node. Possible values:
         *     - LRU: evict containers that have been idle the longest (default)
         *     - RAM: evict as few containers as possible, prioritizing ones with the smallest RAM footprints
         **/
        DECLARE_PROPERTY_NAME(IDLE_CONTAINER_EVICTION_POLICY);

        /** @brief The buffer size used by all internal storage services.
         **/
        DECLARE_PROPERTY_NAME(STORAGE_SERVICES_BUFFER_SIZE);

        /** @brief Whether the download of remote images should be simulated or not (if not, then zero time is assumed).
         *  Setting this to false can create strange re-ordering of initial requests at time zero (because so much happens
         *  exactly at time zero), which likely doesn't matter a lot, but be warned. It's likely better to simulate remote image
         *  downloads and just make the remove bandwidth very high when wanted to essentially ignore remote downloads.
         **/
        DECLARE_PROPERTY_NAME(SIMULATE_REMOTE_IMAGE_DOWNLOADS);

    };

}// namespace wrench

#endif//WRENCH_SERVERLESSCOMPUTESERVICEPROPERTY_H
