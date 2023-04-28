/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simulation/Simulation.h>
#include <wrench/action/Action.h>
#include <wrench/action/MPIAction.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/NotEnoughResources.h>
#include <wrench/services/helper_services/action_executor//ActionExecutor.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>
#include "smpi/smpi.h"


#include <utility>

namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param num_processes: The number of MPI processes that will be started
    * @param num_cores_per_process: number of cores that each process will use.
    * @param lambda_mpi: a lambda with MPI code
    */
    MPIAction::MPIAction(const std::string &name,
                         unsigned long num_processes,
                         unsigned long num_cores_per_process,
                         std::function<void(const std::shared_ptr<ActionExecutor> &action_executor)> lambda_mpi) : Action(name, "mpi_"), num_processes(num_processes), num_cores_per_process(num_cores_per_process), lambda_mpi(lambda_mpi) {
    }

    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void MPIAction::execute(const std::shared_ptr<ActionExecutor> &action_executor) {

        // Make A COPY of the list of usable hosts
        auto resources_ref = action_executor->getActionExecutionService()->getComputeResources();
        std::map<std::string, std::tuple<unsigned long, double>> resources(resources_ref.begin(), resources_ref.end());
        std::vector<std::string> hostnames;
        hostnames.reserve(resources.size());
        for (auto const &h: resources) {
            hostnames.push_back(h.first);
        }

        // Determine the host list (using worst fit)
        std::vector<std::string> hosts;
        while (hosts.size() != this->num_processes) {
            bool added_at_least_one_host = false;
            for (auto const &hostname: hostnames) {
                auto num_cores = std::get<0>(resources[hostname]);
                //                std::cerr << hostname << ": " << num_cores << "\n";
                if (num_cores >= this->num_cores_per_process) {
                    hosts.push_back(hostname);
                    resources[hostname] = std::make_tuple(std::get<0>(resources[hostname]) - this->num_cores_per_process, std::get<1>(resources[hostname]));
                    added_at_least_one_host = true;
                    if (hosts.size() == this->num_processes) {
                        break;
                    }
                }
            }
            if (not added_at_least_one_host) {
                throw ExecutionException(std::make_shared<NotEnoughResources>(this->getJob(), action_executor->getActionExecutionService()->getParentService()));
            }
        }

        // Transform the list of hosts into a list of simgrid hosts
        std::vector<simgrid::s4u::Host *> simgrid_hosts;
        simgrid_hosts.reserve(hosts.size());
        for (auto const &host: hosts) {
            simgrid_hosts.push_back(S4U_Simulation::get_host_or_vm_by_name(host));
        }

        // Do the SMPI thing!!!
        auto barrier = simgrid::s4u::Barrier::create(1 + simgrid_hosts.size());
        // Start actors
        SMPI_app_instance_start(("MPI_Action_" + std::to_string(simgrid::s4u::this_actor::get_pid())).c_str(),
                                MPIProcess(this->lambda_mpi, barrier, action_executor),
                                simgrid_hosts);
        barrier->wait();
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void MPIAction::terminate(const std::shared_ptr<ActionExecutor> &action_executor) {
    }

    /**
     * @brief Returns the action's minimum number of required cores
     * @return Always returns 0 for an MPIAction
     */
    unsigned long MPIAction::getMinNumCores() const {
        return 0;
    }

    /**
     * @brief Returns the action's maximum number of required cores
     * @return Always returns 0 for an MPIAction
     */
    unsigned long MPIAction::getMaxNumCores() const {
        return 0;
    }

    /**
     * @brief Returns the action's minimum required memory footprint
     * @return Always returns 0 for an MPIAction
     */
    double MPIAction::getMinRAMFootprint() const {
        return 0.0;
    }

    /**
     * @brief Returns the number of (MPI) processes to be executed
     * @return a number of processes
     */
    unsigned long MPIAction::getNumProcesses() const {
        return this->num_processes;
    }


    /**
     * @brief Returns the number of cores required by each (MPI) process
     * @return a number of cores
     */
    unsigned long MPIAction::getNumCoresPerProcess() const {
        return this->num_cores_per_process;
    }


}// namespace wrench
