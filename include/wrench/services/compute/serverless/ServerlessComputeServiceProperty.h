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

        /** @brief Whether compute nodes are initially all activated (i.e., usable immediately).
         * ("true": all activated, "false": all de-activated; default: "true")
         **/
        DECLARE_PROPERTY_NAME(COMPUTE_NODES_INITIALLY_ACTIVATED);

        /** @brief The compute node activation overhead; activating multiple nodes at once incurs this overhead only once (default value: "0s")
         *  Examples: "5s", "5000ms", etc.
         **/
        DECLARE_PROPERTY_NAME(COMPUTE_NODE_ACTIVATION_OVERHEAD);

        /** @brief The maximum number of function executions that can be started/running per compute node
         * (default value: "0", which means: same as number of cores per compute node)
         **/
        DECLARE_PROPERTY_NAME(MAX_NUM_CONCURRENT_INVOCATIONS_PER_NODE);

        /** @brief The maximum number of containers that can be starting/running/idling per compute node
         * (default value: "0", which means: same as number of cores per compute node)
         **/
        DECLARE_PROPERTY_NAME(MAX_NUM_CONTAINERS_PER_NODE);

        /** @brief The maximum number of function executions that can execute concurrently in a container
         * (default value: 1)
         **/
        DECLARE_PROPERTY_NAME(MAX_NUM_CONCURRENT_INVOCATIONS_PER_CONTAINER);

        /** @brief The maximum number of outstanding invocations, i.e., invocations that have not yet completed or failed, including
         *         those waiting for image preparation or execution. (default value: "infinity")
         **/
        DECLARE_PROPERTY_NAME(MAX_OUTSTANDING_INVOCATIONS);

        /** @brief The maximum number of outstanding per-function invocations, i.e., invocations that have not yet completed or failed, including
         *         those waiting for image preparation or execution for a particular function. (default value: "infinity")
         **/
        DECLARE_PROPERTY_NAME(MAX_OUTSTANDING_INVOCATIONS_PER_FUNCTION);

        /** @brief The overhead for the compute service to process an incoming request (default value: "0s", default unit: seconds):
         *         Examples: "5s", "5000ms", etc.
         **/
        DECLARE_PROPERTY_NAME(INVOCATION_PROCESSING_OVERHEAD);

        /** @brief The overhead to start a container after the image has been loaded into RAM (default value: "0s", default unit: seconds):
         *         Examples: "5s", "5000ms", etc.
         **/
        DECLARE_PROPERTY_NAME(CONTAINER_STARTUP_OVERHEAD);

        /** @brief The timeout before an idle container is terminated and its disk/resources reclaimed.
         **/
        DECLARE_PROPERTY_NAME(CONTAINER_IDLE_TIMEOUT);

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
