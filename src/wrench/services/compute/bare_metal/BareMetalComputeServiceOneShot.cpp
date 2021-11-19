/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <typeinfo>
#include <map>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.h>
#include <wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/job/PilotJob.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include <wrench/failure_causes/JobTypeNotSupported.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.h>

WRENCH_LOG_CATEGORY(wrench_core_bare_metal_compute_service_one_shot, "Log category for bare_metal_compute_service_on_shot");

namespace wrench {



    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a list of <hostname, num_cores, memory_manager_service> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param ttl: the time-to-live, in seconds (DBL_MAX: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    BareMetalComputeServiceOneShot::BareMetalComputeServiceOneShot(
            std::shared_ptr<CompoundJob> job,
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            double ttl,
            std::shared_ptr<PilotJob> pj,
            std::string suffix, std::shared_ptr<StorageService> scratch_space
    ) : BareMetalComputeService(hostname, compute_resources, property_list, messagepayload_list, ttl, pj, suffix, scratch_space), job(job) {

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

        // Set an alarm for my timely death, if necessary
        if (this->has_ttl) {
            this->death_date = S4U_Simulation::getClock() + this->ttl;
        }


        /** Note that the code below doesn't do any checks at all, the job had better be well-formed **/

        // Add the job to the set of jobs
        this->num_dispatched_actions_for_cjob[this->job] = 0;
        this->current_jobs.insert(this->job);

        // Add all action to the list of actions to run
        for (auto const &action : this->job->getActions()) {
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


}
