/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cfloat>
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "wrench/services/compute/workunit_executor/WorkunitExecutor.h"
#include "wrench/services/compute/workunit_executor/Workunit.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/workflow/job/PilotJob.h"
#include "StandardJobExecutorMessage.h"
#include "wrench/services/helpers/ServiceTerminationDetectorMessage.h"
#include "wrench/services/helpers/ServiceTerminationDetector.h"
#include "wrench/services/helpers/HostStateChangeDetector.h"
#include <wrench/services/helpers/HostStateChangeDetectorMessage.h>
#include <wrench/workflow/failure_causes/HostError.h>

#include <exception>

#include "wrench/util/PointerUtil.h"

WRENCH_LOG_CATEGORY(wrench_core_standard_job_executor, "Log category for Standard Job Executor");

namespace wrench {
    /**
     * @brief Destructor
     */
    StandardJobExecutor::~StandardJobExecutor() {
        this->default_property_values.clear();
    }

    /**
     * @brief Constructor
     *
     * @param simulation: a reference to a simulation object
     * @param callback_mailbox: the mailbox to which a reply will be sent
     * @param hostname: the name of the host on which this service will run (could be the first compute resources - see below)
     * @param job: the standard job to execute
     * @param compute_resources: a non-empty map of <num_cores, memory_manager_service> tuples, indexed by hostname, which represent
     *           the compute resources the job should execute on
     *              - If num_cores == ComputeService::ALL_CORES, then ALL the cores of the host are used
     *              - If memory_manager_service == ComputeService::ALL_RAM, then ALL the ram of the host is used
     * @param scratch_space: the usable scratch storage space  (or nullptr if none)
     * @param part_of_pilot_job: true if the job executor is running within a pilot job
     * @param parent_pilot_job: the parent pilog job, if any
     * @param property_list: a property list
     * @param messagepayload_list: a message payload list
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    StandardJobExecutor::StandardJobExecutor(Simulation *simulation,
                                             std::string callback_mailbox,
                                             std::string hostname,
                                             std::shared_ptr<StandardJob> job,
                                             std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                             std::shared_ptr<StorageService> scratch_space,
                                             bool part_of_pilot_job,
                                             PilotJob *parent_pilot_job,
                                             std::map<std::string, std::string> property_list,
                                             std::map<std::string, double> messagepayload_list
    ) : Service(hostname,
                "standard_job_executor",
                "standard_job_executor") {
        if ((job == nullptr) || (compute_resources.empty())) {
            throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): invalid arguments");
        }

        // Check that hosts exist!
        for (auto h : compute_resources) {
            if (not S4U_Simulation::hostExists(std::get<0>(h))) {
                throw std::invalid_argument(
                        "StandardJobExecutor::StandardJobExecutor(): Host '" + std::get<0>(h) + "' does not exit!");
            }
        }

        // Check that there is at least one core per host but not too many cores
        for (auto host : compute_resources) {
            if (std::get<0>(host.second) == 0) {
                throw std::invalid_argument(
                        "StandardJobExecutor::StandardJobExecutor(): there should be at least one core per host");
            }
            if (std::get<0>(host.second) < ComputeService::ALL_CORES) {
                if (std::get<0>(host.second) > S4U_Simulation::getHostNumCores(host.first)) {
                    throw std::invalid_argument(
                            "StandardJobExecutor::StandardJobExecutor(): host " + host.first +
                            " has only " +
                            std::to_string(S4U_Simulation::getHostNumCores(host.first)) + " cores");
                }
            } else {
                // Set the num_cores to the maximum
                std::get<0>(host.second) = S4U_Simulation::getHostNumCores(host.first);
            }
        }

        // Check that there is at least zero byte of memory_manager_service per host, but not too many bytes
        for (auto host : compute_resources) {
            if (std::get<1>(host.second) < 0) {
                throw std::invalid_argument(
                        "StandardJobExecutor::StandardJobExecutor(): the number of bytes per host should "
                        "be non-negative");
            }
            if (std::get<1>(host.second) < ComputeService::ALL_RAM) {
                double host_memory_capacity = S4U_Simulation::getHostMemoryCapacity(host.first);
                if (std::get<1>(host.second) > host_memory_capacity) {
                    throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): host " + host.first +
                                                " has only " + std::to_string(
                            S4U_Simulation::getHostMemoryCapacity(host.first)) + " bytes of RAM");
                }
            } else {
                // Set the memory_manager_service to the maximum
                std::get<1>(host.second) = S4U_Simulation::getHostMemoryCapacity(host.first);
            }
        }

        // Check that there are enough cores to run the computational tasks
        unsigned long max_min_required_num_cores = 0;
        for (auto task : job->tasks) {
            max_min_required_num_cores = (
                    max_min_required_num_cores < task->getMinNumCores() ? task->getMinNumCores()
                                                                        : max_min_required_num_cores);
        }

        bool enough_cores = false;
        for (auto host : compute_resources) {
            unsigned long num_cores_on_hosts = std::get<0>(host.second);
            if (num_cores_on_hosts == ComputeService::ALL_CORES) {
                num_cores_on_hosts = S4U_Simulation::getHostNumCores(host.first);
            }

            if (num_cores_on_hosts >= max_min_required_num_cores) {
                enough_cores = true;
                break;
            }
        }

        if (!enough_cores) {
            throw std::invalid_argument(
                    "StandardJobExecutor::StandardJobExecutor(): insufficient core resources to run the job");
        }

        // Check that there is enough RAM to run the computational tasks
        double max_required_ram = 0.0;
        for (auto task : job->tasks) {
            max_required_ram = (max_required_ram < task->getMemoryRequirement() ? task->getMemoryRequirement()
                                                                                : max_required_ram);
        }

        bool enough_ram = false;
        for (auto host : compute_resources) {
            if (std::get<1>(host.second) >= max_required_ram) {
                enough_ram = true;
                break;
            }
        }

        if (!enough_ram) {
            throw std::invalid_argument(
                    "StandardJobExecutor::StandardJobExecutor(): insufficient memory_manager_service resources to run the job "
                    "(max_required_ram = " + std::to_string(max_required_ram) + ")");
        }

        // Set instance variables
        this->simulation = simulation;
        this->callback_mailbox = callback_mailbox;
        this->job = job;
        this->scratch_space = scratch_space;
        this->files_stored_in_scratch = {};
        this->part_of_pilot_job = part_of_pilot_job;
        this->parent_pilot_job = parent_pilot_job;

        // set properties
        this->setProperties(this->default_property_values, property_list);
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        // Compute the total number of cores and set initial core availabilities
        this->total_num_cores = 0;
        for (auto host : compute_resources) {
            unsigned long num_cores = std::get<0>(host.second);
            if (num_cores == ComputeService::ALL_CORES) {
                num_cores = simulation->getHostNumCores(host.first);
            }
            this->total_num_cores += num_cores;
            this->core_availabilities.insert(std::make_pair(host.first, num_cores));
        }

        // Compute the total ram and set initial ram availabilities
        this->total_ram = 0.0;
        for (auto host : compute_resources) {
            double ram = std::get<1>(host.second);
            if (ram == ComputeService::ALL_RAM) {
                ram = simulation->getHostMemoryCapacity(host.first);
            }
            this->total_ram += ram;
            this->ram_availabilities.insert(std::make_pair(host.first, ram));
        }

        // Create my compute resources record
        for (auto host : compute_resources) {
            unsigned long num_cores = std::get<0>(host.second);
            if (num_cores == ComputeService::ALL_CORES) {
                num_cores = simulation->getHostNumCores(host.first);
            }
            double ram = std::get<1>(host.second);
            if (ram == ComputeService::ALL_RAM) {
                ram = simulation->getHostMemoryCapacity(std::get<0>(host));
            }
            this->compute_resources.insert(std::make_pair(host.first, std::make_tuple(num_cores, ram)));
        }
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void StandardJobExecutor::cleanup(bool has_returned_from_main, int return_value) {
        // Do the default behavior (which will throw as this is not a fault-tolerant service)
        Service::cleanup(has_returned_from_main, return_value);

        // Cleanup state in case of a restart
        this->finished_workunit_executors.clear();
        this->failed_workunit_executors.clear();
        for (auto wue: this->running_workunit_executors) {
            std::shared_ptr<Workunit> wu = wue->workunit;
            if (wu->task != nullptr) {
                if (wu->task->getInternalState() == WorkflowTask::InternalState::TASK_RUNNING) {
                    wu->task->setInternalState(WorkflowTask::InternalState::TASK_FAILED);
                }
            }
        }

        this->non_ready_workunits.clear();
        this->ready_workunits.clear();
        this->running_workunits.clear();
        this->completed_workunits.clear();
    }


    /**
     * @brief Kill the executor
     * @param job_termination: true if the job was terminated by the submitted, false otherwise
     */
    void StandardJobExecutor::kill(bool job_termination) {
        this->acquireDaemonLock();

        // Kill all Workunit executors
        for (auto const &wue : this->running_workunit_executors) {
            wue->kill(job_termination);
        }

        // Then kill the actor
        this->killActor();

        this->releaseDaemonLock();
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int StandardJobExecutor::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO(
                "New StandardJobExecutor starting (%s) with %d cores and %.2le bytes of RAM over %ld hosts: ",
                this->mailbox_name.c_str(), this->total_num_cores, this->total_ram,
                this->core_availabilities.size());

        for (auto h : this->core_availabilities) {

            WRENCH_INFO("  %s: %ld cores", std::get<0>(h).c_str(), std::get<1>(h));
        }

        if (Simulation::isHostShutdownSimulationEnabled()) {
            // Create the host state monitor
            std::vector<std::string> hosts_to_monitor;
            for (auto const &h : this->compute_resources) {
                hosts_to_monitor.push_back(h.first);
            }
            this->host_state_monitor = std::shared_ptr<HostStateChangeDetector>(
                    new HostStateChangeDetector(this->hostname, hosts_to_monitor, true, false, false,
                                                this->getSharedPtr<Service>(), this->mailbox_name));
            this->host_state_monitor->simulation = this->simulation;
            this->host_state_monitor->start(this->host_state_monitor, true, false); // Daemonized, no auto-restart
        }

        /** Create all Workunits **/
        std::set<std::shared_ptr<Workunit>> all_work_units = Workunit::createWorkunits(this->job);

        /** Put each workunit either in the "non-ready" list or the "ready" list **/
        while (not all_work_units.empty()) {
            auto wu_it = all_work_units.begin();
            if ((*wu_it)->num_pending_parents == 0) {
                auto wu = *wu_it;
                all_work_units.erase(wu_it);
                this->ready_workunits.insert(wu);
            } else {
                auto wu = *wu_it;
                all_work_units.erase(wu_it);
                this->non_ready_workunits.insert(wu);
            }
        }

        /** Main loop **/
        while (true) {
            S4U_Simulation::computeZeroFlop();

            /** Dispatch currently ready workunits, as much as possible  **/
            dispatchReadyWorkunits();

            /** Process workunit completions **/
            if (!processNextMessage()) {
                break;
            }

            /** Detect Termination **/
            if (this->non_ready_workunits.empty() and this->ready_workunits.empty() and
                this->running_workunits.empty()) {
                break;
            }
        }

        StoreListOfFilesInScratch();

        if (not this->part_of_pilot_job) {
            /*** Clean up everything in the scratch space ***/
            cleanUpScratch();
        }

        if (Simulation::isHostShutdownSimulationEnabled()) {
            this->host_state_monitor->kill();
            this->host_state_monitor = nullptr; // Which will release the pointer to this service!
        }

        WRENCH_INFO("Standard Job Executor on host %s cleanly terminating!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Computes the minimum number of cores required to execute a work unit
     * @param wu: the work unit
     * @return a number of cores
     */
    unsigned long StandardJobExecutor::computeWorkUnitMinNumCores(Workunit *wu) {
        return (wu->task != nullptr) ? wu->task->getMinNumCores() : 1;
    }

    /**
     * @brief Computes the desired number of cores required to execute a work unit
     * @param wu: the work unit
     * @return a number of cores
     *
     * @throw std::runtime_error
     */
    unsigned long StandardJobExecutor::computeWorkUnitDesiredNumCores(Workunit *wu) {
        unsigned long desired_num_cores;
        if (wu->task == nullptr) {
            desired_num_cores = 1;

        } else {
            std::string core_allocation_algorithm =
                    this->getPropertyValueAsString(StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM);

            if (core_allocation_algorithm == "maximum") {
                desired_num_cores = wu->task->getMaxNumCores();
            } else if (core_allocation_algorithm == "minimum") {
                desired_num_cores = wu->task->getMinNumCores();
            } else {
                throw std::runtime_error(
                        "StandardjobExecutor::computeWorkUnitDesiredNumCores(): Unknown "
                        "StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM property '"
                        + core_allocation_algorithm + "'");
            }
        }

        return desired_num_cores;
    }

    /**
    * @brief Computes the desired amount of RAM required to execute a work unit
    * @param wu: the work unit
    * @return a number of bytes
    */
    double StandardJobExecutor::computeWorkUnitMinMemory(Workunit *wu) {
        return (wu->task != nullptr) ? wu->task->getMemoryRequirement() : 0.0;
    }

    /**
     * @brief Dispatch ready work units to hosts/cores, while possible
     */
    void StandardJobExecutor::dispatchReadyWorkunits() {
        // If there is no ready work unit, there is nothing to dispatch
        if (this->ready_workunits.empty()) {
            return;
        }

        // Don't kill me while I am doing this!
        this->acquireDaemonLock();

//      std::cerr << "** IN DISPATCH READY WORK UNITS\n";
//      for (auto const &wu : this->ready_workunits) {
//        std::cerr << "WU: num_comp_tasks " << wu->tasks.size() << "\n";
//        for (auto t : wu->tasks) {
//          std::cerr << "    - flops = " << t->getFlops() << ", min_cores = " << t->getMinNumCores() << ", max_cores = " << t->getMaxNumCores() << "\n";
//        }
//      }

        // Get an ordered (by the task selection algorithm) list of the ready workunits
        std::vector<std::shared_ptr<Workunit>> sorted_ready_workunits = sortReadyWorkunits();

//      std::cerr << "** SORTED\n";
//      for (auto wu : sorted_ready_workunits) {
//        std::cerr << "WU: num_comp_tasks " << wu->tasks.size() << "\n";
//        for (auto t : wu->tasks) {
//          std::cerr << "    - flops = " << t->getFlops() << ", min_cores = " << t->getMinNumCores() << ", max_cores = " << t->getMaxNumCores() << "\n";
//        }
//      }

        // Go through the workunits in order of priority and dispatch each them to
        // hosts/cores, if possible
        for (auto wu : sorted_ready_workunits) {
            // Compute the workunit's minimum number of cores, desired number of cores, and minimum amount of ram
            unsigned long minimum_num_cores;
            unsigned long desired_num_cores;
            double required_ram;

            try {
                minimum_num_cores = computeWorkUnitMinNumCores(wu.get());
                desired_num_cores = computeWorkUnitDesiredNumCores(wu.get());
                required_ram = computeWorkUnitMinMemory(wu.get());
            } catch (std::runtime_error &e) {
                this->releaseDaemonLock();
                throw;
            }

            // Find a host on which to run the workunit, and on how many cores
            std::string target_host = "";
            unsigned long target_num_cores = 0;

            WRENCH_INFO(
                    "Looking for a host to run a work unit that needs at least %ld cores, and would like %ld cores, "
                    "and requires %.2ef bytes of RAM",
                    minimum_num_cores, desired_num_cores, required_ram);
            std::string host_selection_algorithm =
                    this->getPropertyValueAsString(StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM);

//        std::cerr << "** FINDING A HOST USING " << host_selection_algorithm << "\n";

            if (host_selection_algorithm == "best_fit") {
                unsigned long target_slack = 0;

                for (auto const &h : this->core_availabilities) {
                    std::string const &hostname = std::get<0>(h);
//              WRENCH_INFO("Looking at host %s", hostname.c_str());

                    // Is the host up?
                    if (not Simulation::isHostOn(hostname)) {
//              WRENCH_INFO("Host is down");
                        continue;
                    }

                    // Does the host have non-zero compute speed?
                    if (Simulation::getHostFlopRate(hostname) <= 0.0) {
                        continue;
                    }

                    // Does the host have enough cores?
                    unsigned long num_available_cores = this->core_availabilities[hostname];
                    if (num_available_cores < minimum_num_cores) {
//              WRENCH_INFO("Not enough cores!");
                        continue;
                    }

                    // Does the host have enough RAM?
                    double available_ram = this->ram_availabilities[hostname];
                    if (available_ram < required_ram) { WRENCH_INFO("Not enough RAM!");
                        continue;
                    }

//            std::cerr << "    HOST COULD WORK \n";

                    unsigned long tentative_target_num_cores = std::min(num_available_cores, desired_num_cores);
                    unsigned long tentative_target_slack = num_available_cores - tentative_target_num_cores;

                    if ((target_host == "") ||
                        (tentative_target_num_cores > target_num_cores) ||
                        ((tentative_target_num_cores == target_num_cores) && (target_slack > tentative_target_slack))) {
//              std::cerr << "YEAH!!!\n";
                        target_host = hostname;
                        target_num_cores = tentative_target_num_cores;
                        target_slack = tentative_target_slack;
                    }
                }
            } else {
                this->releaseDaemonLock();
                throw std::runtime_error("Unknown StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM property '"
                                         + host_selection_algorithm + "'");
            }

            if (target_host.empty()) { // didn't find a suitable host
                WRENCH_INFO("Didn't find a suitable host");
//          std::cerr << "DID NOT FIND A HOST, GOING TO NEXT WORK UNIT\n";
                continue;
            }

//        std::cerr << "FOUND A HOST!!\n";

            // Create a workunit executor!
            WRENCH_INFO("Starting a work unit executor with %ld cores on host %s",
                        target_num_cores, target_host.c_str());

            std::shared_ptr<WorkunitExecutor> workunit_executor = std::shared_ptr<WorkunitExecutor>(
                    new WorkunitExecutor(target_host,
                                         target_num_cores,
                                         required_ram,
                                         this->mailbox_name,
                                         wu,
                                         this->scratch_space,
                                         job,
                                         this->getPropertyValueAsDouble(
                                                 StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD),
                                         this->getPropertyValueAsBoolean(
                                                 StandardJobExecutorProperty::SIMULATE_COMPUTATION_AS_SLEEP)
                    ));

            workunit_executor->simulation = this->simulation;
            try {
                workunit_executor->start(workunit_executor, true, false); // Daemonized, no auto-restart
                // This is an error on the target host!!
            } catch (std::shared_ptr<HostError> &e) {
                this->releaseDaemonLock();
                throw std::runtime_error(
                        "bare_metal::dispatchReadyWorkunits(): got a host error on the target host - this shouldn't happen");
            }

            // Start a failure detector for this workunit executor (which will send me a message in case the
            // work unit executor has died)
            auto failure_detector = std::shared_ptr<ServiceTerminationDetector>(
                    new ServiceTerminationDetector(this->hostname, workunit_executor, this->mailbox_name, true, false));
            failure_detector->simulation = this->simulation;
            failure_detector->start(failure_detector, true, false); // Daemonized, no auto-restart

            // Update core availabilities
            this->core_availabilities[target_host] -= target_num_cores;
            // Update RAM availabilities
            this->ram_availabilities[target_host] -= required_ram;

            // Update data structures
            this->running_workunit_executors.insert(workunit_executor);

            for (auto wu_it = this->ready_workunits.begin(); wu_it != this->ready_workunits.end(); wu_it++) {
                if ((*wu_it) == wu) {
                    auto tomove = *wu_it;
                    this->ready_workunits.erase(wu_it);
                    this->running_workunits.insert(tomove);
                    break;
                }
            }
        }

        sorted_ready_workunits.clear();

        this->releaseDaemonLock();
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @param timeout: timeout value in seconds
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool StandardJobExecutor::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        } catch (std::shared_ptr<FatalFailure> &cause) { WRENCH_INFO(
                    "Got a Unknown Failure during a communication... likely this means we're all done. Aborting");
            return false;
        }

        if (message == nullptr) { WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<HostHasTurnedOnMessage *>(message.get())) {
            // Do nothing, just wake up
            return true;
        } else if (auto msg = dynamic_cast<HostHasChangedSpeedMessage *>(message.get())) {
            // Do nothing, just wake up
            return true;
        } else if (auto msg = dynamic_cast<WorkunitExecutorDoneMessage *>(message.get())) {
            processWorkunitExecutorCompletion(msg->workunit_executor, msg->workunit);
            return true;
        } else if (auto msg = dynamic_cast<WorkunitExecutorFailedMessage *>(message.get())) {
            processWorkunitExecutorFailure(msg->workunit_executor, msg->workunit, msg->cause);
            return false; // We should exit since we've killed everything

        } else if (auto msg = dynamic_cast<ServiceHasCrashedMessage *>(message.get())) {
            auto service = msg->service;
            auto workunit_executor = std::dynamic_pointer_cast<WorkunitExecutor>(service);
            if (not workunit_executor) {
                throw std::runtime_error(
                        "Received a FailureDetectorServiceHasFailedMessage message, but that service is not a WorkUnitExecutor!");
            }
            processWorkunitExecutorCrash(workunit_executor);
            return true;
        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }


    /**
     * @brief Process a workunit completion
     * @param workunit_executor: the workunit executor that completed the work unit
     * @param workunit: the workunit
     *
     * @throw std::runtime_error
     */
    void StandardJobExecutor::processWorkunitExecutorCompletion(std::shared_ptr<WorkunitExecutor> workunit_executor,
                                                                std::shared_ptr<Workunit> workunit) {
        // Don't kill me while I am doing this
        this->acquireDaemonLock();

        // Update core availabilities
        this->core_availabilities[workunit_executor->getHostname()] += workunit_executor->getNumCores();
        // Update RAM availabilities
        this->ram_availabilities[workunit_executor->getHostname()] += workunit_executor->getMemoryUtilization();

        // Remove the workunit executor from the workunit executor list
        for (auto it = this->running_workunit_executors.begin(); it != this->running_workunit_executors.end(); it++) {
            if ((*it) == workunit_executor) {
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_workunit_executors),
                                                       &(this->finished_workunit_executors));
                break;
            }
        }

        // Find the workunit in the running workunit queue
        bool found_it = false;
        for (auto it = this->running_workunits.begin(); it != this->running_workunits.end(); it++) {
            if ((*it) == workunit) {
                auto tomove = *it;
                this->running_workunits.erase(it);
                this->completed_workunits.insert(tomove);
                found_it = true;
                break;
            }
        }
        if (!found_it) {
            throw std::runtime_error(
                    "StandardJobExecutor::processWorkunitExecutorCompletion(): couldn't find a recently completed "
                    "workunit in the running workunit list");
        }

        // Process task completions, if any
        if (workunit->task != nullptr) { WRENCH_INFO("A workunit executor completed task %s (and its state is: %s)",
                                                     workunit->task->getID().c_str(),
                                                     WorkflowTask::stateToString(
                                                             workunit->task->getInternalState()).c_str());

            // Increase the "completed tasks" count of the job
            this->job->incrementNumCompletedTasks();
        }

        // Send the callback to the originator if the job has completed
        if ((this->non_ready_workunits.empty()) &&
            (this->ready_workunits.empty()) &&
            (this->running_workunits.empty())) {

//            WRENCH_INFO("NOTIFYING BACK SAYING  'COMPLETED' for job %s:", this->job->getName().c_str());
            try {
                S4U_Mailbox::putMessage(
                        this->callback_mailbox,
                        new StandardJobExecutorDoneMessage(
                                this->job,
                                this->getSharedPtr<StandardJobExecutor>(),
                                this->getMessagePayloadValue(
                                        StandardJobExecutorMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) { WRENCH_INFO("Failed to send the callback... oh well");
                this->releaseDaemonLock();
                return;
            }
        } else {
            // Otherwise, update children
            for (auto child : workunit->children) {
                child->num_pending_parents--;
                if (child->num_pending_parents == 0) {
                    // Make the child's tasks ready
                    if (child->task != nullptr) {
                        if (child->task->getInternalState() != WorkflowTask::InternalState::TASK_READY) {
                            throw std::runtime_error(
                                    "StandardJobExecutor::processWorkunitExecutorCompletion(): Weird task state " +
                                    std::to_string(child->task->getInternalState()) + " for task " +
                                    child->task->getID());
                        }
                    }

                    // Find the child working in the non-ready  queue
                    found_it = false;
                    for (auto it = this->non_ready_workunits.begin(); it != this->non_ready_workunits.end(); it++) {
                        if ((*it) == child) {
                            // Move it to the ready  queue
                            auto tomove = *it;
                            this->non_ready_workunits.erase(it);
                            this->ready_workunits.insert(tomove);
                            found_it = true;
                            break;
                        }
                    }
                    if (!found_it) {
                        throw std::runtime_error(
                                "bare_metal::processWorkCompletion(): couldn't find non-ready child in non-ready set!");
                    }

                }
            }
        }

        this->releaseDaemonLock();
    }

    /**
     * @brief Process a work failure
     * @param worker_thread: the worker thread that did the work
     * @param workunit: the workunit
     * @param cause: the cause of the failure
     */
    void StandardJobExecutor::processWorkunitExecutorFailure(std::shared_ptr<WorkunitExecutor> workunit_executor,
                                                             std::shared_ptr<Workunit> workunit,
                                                             std::shared_ptr<FailureCause> cause) {
        // Don't kill me while I am doing this
        this->acquireDaemonLock();

        WRENCH_INFO("A workunit executor has failed to complete a workunit on behalf of job '%s'",
                    this->job->getName().c_str());

        // Update core availabilities
        this->core_availabilities[workunit_executor->getHostname()] += workunit_executor->getNumCores();
        // Update RAM availabilities
        this->ram_availabilities[workunit_executor->getHostname()] += workunit_executor->getMemoryUtilization();

        // Remove the workunit executor from the workunit executor list and put it in the failed list
        for (auto it = this->running_workunit_executors.begin(); it != this->running_workunit_executors.end(); it++) {
            if ((*it) == workunit_executor) {
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_workunit_executors),
                                                       &(this->failed_workunit_executors));
                break;
            }
        }

        // Remove the work from the running work queue
        bool found_it = false;
        for (auto it = this->running_workunits.begin(); it != this->running_workunits.end(); it++) {
            if ((*it) == workunit) {
                this->running_workunits.erase(it);
                found_it = true;
                break;
            }
        }
        if (!found_it) {
            this->releaseDaemonLock();
            throw std::runtime_error(
                    "StandardJobExecutor::processWorkunitExecutorFailure(): couldn't find a recently failed "
                    "workunit in the running workunit list");
        }

        // Deal with running work units!
        for (auto const &wu : this->running_workunits) {
            if ((not wu->post_file_copies.empty()) || (not wu->pre_file_copies.empty())) {
                this->releaseDaemonLock();
                throw std::runtime_error(
                        "StandardJobExecutor::processWorkunitExecutorFailure(): trying to cancel a running workunit "
                        "that's doing some file copy operations - not supported (for now)");
            }
            // find the workunit executor  that's doing the work and kill it (lame iteration)
            for (auto const &wue : this->running_workunit_executors) {
                if (wue->workunit == wu) {
                    wue->kill(false);
                    break;
                }
            }
        }

        this->releaseDaemonLock();

        // Send the notification back
        try {
            S4U_Mailbox::putMessage(
                    this->callback_mailbox,
                    new StandardJobExecutorFailedMessage(
                            this->job,
                            this->getSharedPtr<StandardJobExecutor>(),
                            cause,
                            this->getMessagePayloadValue(
                                    StandardJobExecutorMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            // do nothing
        }
    }

    /**
     * @brief Process a WorkunitExecutor crash
     * @param workunit_executor: the WorkunitExecutor that has crashed
     */
    void StandardJobExecutor::processWorkunitExecutorCrash(std::shared_ptr<WorkunitExecutor> workunit_executor) {
        WRENCH_INFO("Handling a WorkunitExecutor crash!");

        // Look for the workunit executor
        bool found_it = false;
        for (auto const &wue : this->running_workunit_executors) {
            if (wue == workunit_executor) {
                auto tmp = wue;
                this->running_workunit_executors.erase(wue);
                this->failed_workunit_executors.insert(tmp);
                found_it = true;
                break;
            }
        }
        if (not found_it) {
            throw std::runtime_error(
                    "StandardJobExecutor::processWorkunitExecutorCrash(): Received a WorkunitExecutorCrash message "
                    "for a non-running WorkunitExecutor!");
        }

        // Get the scratch files that executor may have generated
        for (auto &f : workunit_executor->getFilesStoredInScratch()) {
            this->files_stored_in_scratch.insert(f);
        }

        auto workunit = workunit_executor->workunit;

        // Update Core availabilities
        this->core_availabilities[workunit_executor->getHostname()] -= workunit_executor->getNumCores();
        // Update RAM availabilities and running thread counts
        if (workunit->task) {
            this->ram_availabilities[workunit_executor->getHostname()] += workunit->task->getMemoryRequirement();
            // Reset the internal task state to READY (it may have been completed actually, but we just redo the whole workunit)
            workunit->task->setInternalState(WorkflowTask::InternalState::TASK_READY);
        }

        // Put the WorkUnit back in the ready list (at the end)
        WRENCH_INFO("Putting task back in the ready queue");
        this->running_workunits.erase(workunit);
        this->ready_workunits.insert(workunit);
    }

    /**
     * @brief Sort the a list of ready workunits based on the TASK_SELECTION_ALGORITHM property
     *
     * @return a sorted vector of ready tasks
     */
    std::vector<std::shared_ptr<Workunit>> StandardJobExecutor::sortReadyWorkunits() {
//      std::cerr << "In sortReadyWorkunits()\n";

        std::vector<std::shared_ptr<Workunit>> sorted_workunits;

        for (auto const &wu : this->ready_workunits) {
            sorted_workunits.push_back(wu);
//        std::cerr << "WORKUNITS.GET = " << wu.get() << ": " << wu.get()->tasks.size() << "\n";
        }

//      std::cerr << "SORTED LENGTH = " << sorted_workunits.size() << "\n";

        std::string selection_algorithm =
                this->getPropertyValueAsString(StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM);

//      std::cerr << "SELECT ALG = " << selection_algorithm << "\n";

        // using function as comp
        std::sort(sorted_workunits.begin(), sorted_workunits.end(),
                  [selection_algorithm](const std::shared_ptr<Workunit> wu1,
                                        const std::shared_ptr<Workunit> wu2) -> bool {
//                    std::cerr << "IN LAMBDA1: " << wu1 << "  " << wu2 << "\n";
//                    std::cerr << "IN LAMBDA2: " << wu1->tasks.size() << "  " << wu2->tasks.size() << "\n";
                      // Non-computational workunits have higher priority

                      if (wu1->task == nullptr and wu2->task == nullptr) {
                          return ((uintptr_t) wu1.get() > (uintptr_t) wu2.get());
                      }
                      if (wu1->task == nullptr) {
                          return true;
                      }
                      if (wu2->task == nullptr) {
                          return false;
                      }

                      if (selection_algorithm == "maximum_flops") {
                          if (wu1->task->getFlops() == wu2->task->getFlops()) {
                              return (wu1->task->getID() > wu2->task->getID());
                          }
                          return (wu1->task->getFlops() > wu2->task->getFlops());

                      } else if (selection_algorithm == "maximum_minimum_cores") {
                          if (wu1->task->getMinNumCores() == wu2->task->getMinNumCores()) {
                              return (wu1->task->getID() > wu2->task->getID());
                          }
                          return (wu1->task->getMinNumCores() > wu2->task->getMinNumCores());

                      } else if (selection_algorithm == "minimum_top_level") {
                          if (wu1->task->getTopLevel() == wu2->task->getTopLevel()) {
                              return (wu1->task->getID() > wu2->task->getID());
                          }
                          return (wu1->task->getTopLevel() < wu2->task->getTopLevel());
                      } else {
                          throw std::runtime_error(
                                  "Unknown StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM property '"
                                  + selection_algorithm + "'");
                      }
                  });

        return sorted_workunits;
    }

    /**
     * @brief Clears the scratch space
     */
    void StandardJobExecutor::cleanUpScratch() {
        if (this->scratch_space != nullptr) {
            /** Perform scratch cleanup */
            for (auto f : files_stored_in_scratch) {
                try {
                    StorageService::deleteFile(f, FileLocation::LOCATION(this->scratch_space,
                                                                         "/scratch/" + job->getName()));
                } catch (WorkflowExecutionException &e) {
                    throw;
                }
            }
        }
    }

    /**
     * @brief Store the list of files available in scratch
     */
    void StandardJobExecutor::StoreListOfFilesInScratch() {
        // First fetch all the files stored in scratch by all the workunit executors running inside a standardjob
        // Files in scratch by finished workunit executors
        for (auto it = this->finished_workunit_executors.begin(); it != this->finished_workunit_executors.end(); it++) {
            std::set<WorkflowFile *> files_in_scratch_by_single_workunit = (*it)->getFilesStoredInScratch();
            this->files_stored_in_scratch.insert(files_in_scratch_by_single_workunit.begin(),
                                                 files_in_scratch_by_single_workunit.end());
        }
        // Files in scratch by failed workunit executors
        for (auto it = this->failed_workunit_executors.begin(); it != this->failed_workunit_executors.end(); it++) {
            std::set<WorkflowFile *> files_in_scratch_by_single_workunit = (*it)->getFilesStoredInScratch();
            this->files_stored_in_scratch.insert(files_in_scratch_by_single_workunit.begin(),
                                                 files_in_scratch_by_single_workunit.end());
        }
    }

    /**
     * @brief Get the set of files stored in scratch space during the standard job's execution
     *
     * @return a set of files
     */
    std::set<WorkflowFile *> StandardJobExecutor::getFilesInScratch() {
        return this->files_stored_in_scratch;
    }

    /**
     * @brief Get the executor's job
     * @return a standard job
     */
    std::shared_ptr<StandardJob> StandardJobExecutor::getJob() {
        return this->job;
    }

    /**
     * @brief Get the executor's compute resources
     * @return a set of compute resources as <hostname, num cores, bytes of RAM> tuples
     */
    std::map<std::string, std::tuple<unsigned long, double>> StandardJobExecutor::getComputeResources() {
        return this->compute_resources;
    }

}
