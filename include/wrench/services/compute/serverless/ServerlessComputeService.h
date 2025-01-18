/**
* Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SERVERLESSCOMPUTESERVICE_H
#define SERVERLESSCOMPUTESERVICE_H

#include "wrench/services/compute/ComputeService.h"

namespace wrench {

/**
     * @brief A batch-scheduled compute service that manages a set of compute hosts and
     *        controls access to their resource via a batch queue.
     *
     *        In the current implementation of
     *        this service, like for many of its real-world counterparts, memory_manager_service
     *        partitioning among jobs onq the same host is not handled.  When multiple jobs share hosts,
     *        which can happen when jobs require only a few cores per host and can thus
     *        be co-located on the same hosts in a non-exclusive fashion,
     *        each job simply runs as if it had access to the
     *        full RAM of each compute host it is scheduled on. The simulation of these
     *        memory_manager_service contended scenarios is thus, for now, not realistic as there is no simulation
     *        of the effects
     *        of memory_manager_service sharing (e.g., swapping).
     */
    class ServerlessComputeService : public ComputeService {

    public:
        ServerlessComputeService(const std::string &hostname,
                            std::vector<std::string> compute_hosts,
                            std::string scratch_space_mount_point,
                            WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                            WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;


    private:
        int main() override;

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string> &service_specific_args) override{
            throw std::runtime_error("not implemented");
        }

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override {
            throw std::runtime_error("not implemented");
        }

        std::map<std::string, double> constructResourceInformation(const std::string &key) override {
            throw std::runtime_error("not implemented");
        }


        std::vector<std::string> _compute_hosts;

    };

};

#endif //SERVERLESSCOMPUTESERVICE_H
