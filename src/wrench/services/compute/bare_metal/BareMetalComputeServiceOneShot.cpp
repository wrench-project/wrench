/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include <utility>

#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.h>
#include <wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/job/PilotJob.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>

WRENCH_LOG_CATEGORY(wrench_core_bare_metal_compute_service_one_shot, "Log category for bare_metal_compute_service_on_shot");

namespace wrench {


    /**
     * @brief Internal constructor
     *
     * @param job: the compound job to be executed
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a list of <hostname, num_cores, memory_manager_service> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     * @param scratch_space: the scratch space to use
     *
     * @throw std::invalid_argument
     */
    BareMetalComputeServiceOneShot::BareMetalComputeServiceOneShot(
            std::shared_ptr<CompoundJob> job,
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
            std::shared_ptr<PilotJob> pj,
            const std::string &suffix, std::shared_ptr<StorageService> scratch_space) : BareMetalComputeService(hostname, std::move(compute_resources), std::move(property_list), std::move(messagepayload_list), std::move(pj), suffix, std::move(scratch_space)), job(std::move(job)) {
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BareMetalComputeServiceOneShot::main() {
        this->state = Service::UP;


        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO("New OneShot BareMetal Compute Service starting");

        // Start the ActionExecutionService
        this->action_execution_service->setParentService(this->getSharedPtr<Service>());
        this->action_execution_service->setSimulation(this->simulation);
        this->action_execution_service->start(this->action_execution_service, true, false);

        /** Note that the code below doesn't do any checks at all, the job had better be well-formed **/

        // Add the job to the set of jobs
        this->num_dispatched_actions_for_cjob[this->job] = 0;
        this->current_jobs.insert(this->job);

        // Add all action to the list of actions to run
        for (auto const &action: this->job->getActions()) {
            if (action->getState() == Action::State::READY) {
                this->ready_actions.push_back(action);
            } else {
                this->not_ready_actions.insert(action);
            }
        }

        /** Main loop **/
        dispatchReadyActions();
        while (this->processNextMessage()) {
            dispatchReadyActions();
        }

        WRENCH_INFO("One-Shot BareMetalService terminating cleanly!");
        return this->exit_code;
    }

}// namespace wrench
