/**
 * Copyright (c) 2017-2022. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <typeinfo>
#include <map>
#include <wrench/util/PointerUtil.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <utility>

#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h>
#include <wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/action/Action.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include <wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h>

WRENCH_LOG_CATEGORY(wrench_core_action_scheduler, "Log category for Action Scheduler");

namespace wrench {
    /**
     * @brief Destructor
     */
    ActionExecutionService::~ActionExecutionService() {
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void ActionExecutionService::cleanup(bool has_returned_from_main, int return_value) {

        //        this->release_held_mutexes();

        // Clean up state in case of a restart
        if (this->isSetToAutoRestart()) {
            for (const auto &host: this->compute_resources) {
                this->ram_availabilities.insert(
                        std::make_pair(host.first, S4U_Simulation::getHostMemoryCapacity(host.first)));
                this->running_thread_counts.insert(std::make_pair(host.first, 0));
            }
        }

        this->all_actions.clear();
        this->ready_actions.clear();
        this->action_executors.clear();
    }

    /**
     * @brief Helper static method to parse resource specifications to the <cores,ram> format
     * @param spec: specification string
     * @return a <host, core> tuple
     */
    static std::tuple<simgrid::s4u::Host *, unsigned long> parseResourceSpec(const std::string &spec) {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, spec, boost::is_any_of(":"));
        switch (tokens.size()) {
            case 1:// "num_cores" or "hostname"
            {
                unsigned long num_threads;
                if (sscanf(tokens[0].c_str(), "%lu", &num_threads) != 1) {
                    return std::make_tuple(S4U_Simulation::get_host_or_vm_by_name(tokens[0]), 0);
                } else {
                    return std::make_tuple(nullptr, num_threads);
                }
            }
            case 2:// "hostname:num_cores"
            {
                unsigned long num_threads;
                if (sscanf(tokens[1].c_str(), "%lu", &num_threads) != 1) {
                    throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
                }
                return std::make_tuple(S4U_Simulation::get_host_or_vm_by_name(tokens[0]), num_threads);
            }
            default: {
                throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
            }
        }
    }

    /**
     * @brief Submit an action to the action execution service service
     * @param action: an action
     */
    void ActionExecutionService::submitAction(const std::shared_ptr<Action> &action) {
        assertServiceIsUp();

        if (action->getState() != Action::State::READY) {
            throw std::runtime_error("Can only submit a ready action to the ActionExecutionService");
        }

#if 0// These checks should have ALL happened before

        // Check that service-specific args that are provided are well-formatted
        std::string action_name = action->getName();
        auto service_specific_args = action->getJob()->getServiceSpecificArguments();

        if ((service_specific_args.find(action_name) != service_specific_args.end()) and
            (not service_specific_args.at(action_name).empty())) {
            std::tuple<std::string, unsigned long> parsed_spec;
                parsed_spec = parseResourceSpec(service_specific_args.at(action_name));

            std::string target_host = std::get<0>(parsed_spec);
            unsigned long target_num_cores = std::get<1>(parsed_spec);

            if (not target_host.empty()) {
                if (this->compute_resources.find(target_host) == this->compute_resources.end()) {
                    throw std::invalid_argument(
                            "Invalid service-specific argument '" + service_specific_args.at(action_name) +
                            "' for action '" + action_name + "': no such host");
                }
            }

            if (target_num_cores > 0) {
                if (target_num_cores < action->getMinNumCores()) {
                    throw std::invalid_argument(
                            "Invalid service-specific argument '" + service_specific_args.at(action_name) +
                            "' for action '" +
                            action_name + "': the action requires at least " + std::to_string(action->getMinNumCores()) +
                            " cores");
                }
                if (target_num_cores > action->getMaxNumCores()) {
                    throw std::invalid_argument(
                            "Invalid service-specific argument '" + service_specific_args.at(action_name) +
                            "' for action '" +
                            action_name + "': the action can use at most " + std::to_string(action->getMaxNumCores()) +
                            " cores");
                }
            }
        }
#endif

        // At this point, there may still be insufficient resources to run the action, but that will
        // be handled later (and some ExecutionError with a "not enough resources" FailureCause
        // may be generated).

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
                new ActionExecutionServiceSubmitActionRequestMessage(
                        answer_commport, action,
                        0.0));

        // Get the answer
        auto msg = answer_commport->getMessage<ActionExecutionServiceSubmitActionAnswerMessage>(
                this->network_timeout,
                "ActionExecutionService::submitActions(): Received an");
        // If not a success, throw an exception
        if (not msg->success) {
            throw ExecutionException(msg->cause);
        }
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a map of <num_cores, memory> tuples, indexed by host, which represents
     *        the compute resources available to this service.
     *          - use num_cores = ComputeService::ALL_CORES to use all cores available on the host
     *          - use memory_manager_service = ComputeService::ALL_RAM to use all RAM available on the host
     * @param parent_service: the parent compute service (nullptr if not known at this time)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    ActionExecutionService::ActionExecutionService(
            const std::string &hostname,
            const std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> &compute_resources,
            std::shared_ptr<Service> parent_service,
            const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
            const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) : Service(hostname,
                                                                                 "action_execution_service") {
        // Set default and specified properties
        this->setProperties(this->default_property_values, property_list);

        // Validate that properties are correct
        this->validateProperties();

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        // Check that there is at least one core per host and that hosts have enough cores
        if (compute_resources.empty()) {
            throw std::invalid_argument(
                    "ActionExecutionService::ActionExecutionService(): the resource list is empty");
        }
        for (auto host: compute_resources) {
            unsigned long requested_cores = std::get<0>(host.second);
            unsigned long available_cores;
            try {
                available_cores = host.first->get_core_count();
            } catch (std::runtime_error &) {
                throw std::invalid_argument(
                        "ActionExecutionService::ActionExecutionService(): Host '" + host.first->get_name() + "' does not exist");
            }
            if (requested_cores == ComputeService::ALL_CORES) {
                requested_cores = available_cores;
            }
            if (requested_cores == 0) {
                throw std::invalid_argument(
                        "ActionExecutionService::ActionExecutionService(): at least 1 core should be requested");
            }
            if (requested_cores > available_cores) {
                throw std::invalid_argument(
                        "ActionExecutionService::ActionExecutionService(): " + host.first->get_name() + "only has " +
                        std::to_string(available_cores) + " cores but " +
                        std::to_string(requested_cores) + " are requested");
            }

            sg_size_t requested_ram = std::get<1>(host.second);
            sg_size_t available_ram = S4U_Simulation::getHostMemoryCapacity(host.first);

            if (requested_ram == ComputeService::ALL_RAM) {
                requested_ram = available_ram;
            }

            if (requested_ram > available_ram) {
                throw std::invalid_argument(
                        "ActionExecutionService::ActionExecutionService(): host " + host.first->get_name() + "only has " +
                        std::to_string(available_ram) + " bytes of RAM but " +
                        std::to_string(requested_ram) + " are requested");
            }

            this->compute_resources[host.first] = std::make_tuple(requested_cores, requested_ram);
        }


        // Compute the total number of cores and set initial ram availabilities
        for (auto const &host: this->compute_resources) {
            this->ram_availabilities[host.first] = std::get<1>(this->compute_resources[host.first]);
            this->running_thread_counts[host.first] = 0;
        }

        this->parent_service = std::move(parent_service);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int ActionExecutionService::main() {
        if (this->parent_service == nullptr) {
            throw std::runtime_error("ActionExecutionService::main(): parent service not set - please call setParentService before starting this service");
        }

        this->state = Service::UP;

        this->num_hosts_turned_on = this->compute_resources.size();

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        // Print some logging
        WRENCH_INFO("New Action Execution Service started by %s on %zu hosts",
                    this->parent_service->getName().c_str(), this->compute_resources.size());
        std::string msg = "\n";
        for (auto cr: this->compute_resources) {
            auto host = cr.first;
            auto num_cores = std::get<0>(cr.second);
            auto ram = std::get<1>(cr.second);
            msg += "  - " + host->get_name() + ": " + std::to_string(num_cores) + " cores; " + std::to_string(ram / 1000000000) +
                   " GB of RAM\n";
        }

        // Create and start the host state monitor if necessary
        if (Simulation::isEnergySimulationEnabled() or Simulation::isHostShutdownSimulationEnabled()) {
            // Create the host state monitor
            std::vector<simgrid::s4u::Host *> hosts_to_monitor;
            for (auto const &h: this->compute_resources) {
                hosts_to_monitor.push_back(h.first);
            }
            this->host_state_change_monitor = std::make_shared<HostStateChangeDetector>(
                this->hostname, hosts_to_monitor, true, true, true,
                this->getSharedPtr<Service>(), this->commport);
            this->host_state_change_monitor->setSimulation(this->simulation_);
            this->host_state_change_monitor->start(this->host_state_change_monitor, true,
                                                   false);// Daemonized, no auto-restart
        }

        /** Main loop **/
        while (this->processNextMessage()) {
            /** Dispatch ready actions **/
            this->dispatchReadyActions();
        }

        // Kill the host state monitor if necessary
        if (Simulation::isEnergySimulationEnabled() or Simulation::isHostShutdownSimulationEnabled()) {
            this->host_state_change_monitor->kill();
            this->host_state_change_monitor = nullptr;// Which will release the pointer to this service!
        }

        WRENCH_INFO("ActionExecutionService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return this->exit_code;
    }

    /**
     * @brief helper function to figure out where/how an action should run
     *
     * @param action: the action for which this allocation is being computed
     * @param required_host: the required host per service-specific arguments (nullptr means: choose one)
     * @param required_num_cores: the required number of cores per service-specific arguments (0 means: choose a number)
     * @param hosts_to_avoid: a list of hosts to not even consider
     * @return an allocation
     */
    std::tuple<simgrid::s4u::Host *, unsigned long> ActionExecutionService::pickAllocation(
            const std::shared_ptr<Action> &action,
            const simgrid::s4u::Host *required_host,
            const unsigned long required_num_cores,
            std::set<simgrid::s4u::Host *> &hosts_to_avoid) {

        // Compute possible hosts
        std::set<simgrid::s4u::Host *> possible_hosts;
        simgrid::s4u::Host *new_host_to_avoid = nullptr;
        sg_size_t new_host_to_avoid_ram_capacity = 0;
        for (auto const &r: this->compute_resources) {

            // If there is a required host, then don't even look at others
            if (required_host != nullptr and (r.first != required_host)) {
                continue;
            }

            // If the host is down, then don't look at it
            if (not r.first->is_on()) {
                continue;
            }

            // If the host has compute speed zero, then don't look at it
            if (r.first->get_speed() <= 0.0) {
                continue;
            }

            unsigned long available_cores = std::get<0>(r.second) - this->running_thread_counts[r.first];
            if (required_num_cores == 0) {
                if (available_cores < action->getMinNumCores()) {
                    continue;
                }
            }
            if (required_num_cores != 0) {
                if (available_cores < required_num_cores) {
                    continue;
                }
            }
            if ((action->getMinRAMFootprint() > 0) and (hosts_to_avoid.find(r.first) != hosts_to_avoid.end())) {
                continue;
            }
            if (this->ram_availabilities[r.first] < action->getMinRAMFootprint()) {
                if (new_host_to_avoid == nullptr) {
                    new_host_to_avoid = r.first;
                    new_host_to_avoid_ram_capacity = this->ram_availabilities[r.first];
                } else {
                    if (this->ram_availabilities[r.first] > new_host_to_avoid_ram_capacity) {
                        // Make sure we "Avoid" the host with the most RAM (as it might become usable sooner)
                        new_host_to_avoid = r.first;
                        new_host_to_avoid_ram_capacity = this->ram_availabilities[r.first];
                    }
                }
                continue;
            }

            possible_hosts.insert(r.first);
        }

        // If none, then reply with an empty tuple
        if (possible_hosts.empty()) {
            // Host to avoid is the one with the lowest ram availability
            if (new_host_to_avoid != nullptr) {
                hosts_to_avoid.insert(new_host_to_avoid);
            }
            return std::make_tuple(nullptr, 0);
        }

        // Select the "best" host
        double lowest_load = DBL_MAX;
        simgrid::s4u::Host *picked_host = nullptr;
        unsigned long picked_num_cores = 0;
        for (auto const &h: possible_hosts) {
            unsigned long num_running_threads = this->running_thread_counts[h];
            unsigned long num_cores = std::get<0>(this->compute_resources[h]);
            double flop_rate = h->get_speed();
            unsigned long used_num_cores;
            if (required_num_cores == 0) {
                used_num_cores = std::min(num_cores, action->getMaxNumCores());// as many cores as possible
            } else {
                used_num_cores = required_num_cores;
            }
            // A totally heuristic load estimate
            const double load = ((static_cast<double>(num_running_threads + used_num_cores) / static_cast<double>(num_cores))) /
                          (flop_rate / (1000.0 * 1000.0 * 1000.0));
            if (load < lowest_load) {
                lowest_load = load;
                picked_host = h;
                picked_num_cores = used_num_cores;
            }
        }

        return std::make_tuple(picked_host, picked_num_cores);
    }

    /**
     * @brief: Dispatch ready work units
     */
    void ActionExecutionService::dispatchReadyActions() {

        if (this->ready_actions.empty()) {
            return;
        }

        // Don't kill me while I am doing this
        this->acquireDaemonLock();

        std::set<std::shared_ptr<Action>> dispatched_actions;

        // Due to a previously considered actions not being
        // able to run on that host due to RAM, and because we don't
        // allow non-zero-ram tasks to jump ahead of other tasks
        std::set<simgrid::s4u::Host *> no_longer_considered_hosts;

        for (auto const &action: this->ready_actions) {
            std::string picked_host;
            simgrid::s4u::Host *target_host = nullptr;
            unsigned long target_num_cores;
            sg_size_t required_ram;

            std::tuple<simgrid::s4u::Host *, unsigned long> allocation =
                    pickAllocation(action,
                                   std::get<0>(this->action_run_specs[action]),
                                   std::get<1>(this->action_run_specs[action]),
                                   no_longer_considered_hosts);
            required_ram = action->getMinRAMFootprint();
            target_host = std::get<0>(allocation);
            target_num_cores = std::get<1>(allocation);


            // If we didn't find a host, forget it
            if (target_host == nullptr) {
                continue;
            }
            //            WRENCH_INFO("ALLOC %s: %s %ld %lf", action->getName().c_str(), target_host.c_str(), target_num_cores, required_ram);

            /** Dispatch it **/
            // Create an action executor on the target host
            auto action_executor = std::shared_ptr<ActionExecutor>(
                    new ActionExecutor(target_host->get_name(),
                                       target_num_cores,
                                       required_ram,
                                       this->getPropertyValueAsTimeInSecond(ActionExecutionServiceProperty::THREAD_CREATION_OVERHEAD),
                                       this->getPropertyValueAsBoolean(ActionExecutionServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP),
                                       this->commport,
                                       nullptr,
                                       action,
                                       this->getSharedPtr<ActionExecutionService>()));

            action_executor->setSimulation(this->simulation_);
            try {
                action_executor->start(action_executor, true, false);// Daemonized, no auto-restart
            } catch (ExecutionException &) {
                // This is an error on the target host!!
                throw std::runtime_error(
                        "ActionSchedule::dispatchReadyActions(): got a host error on the target host - this shouldn't happen");
            }

            // Start a failure detector for this action executor (which will send me a message in case the
            // action executor has died)
            auto failure_detector = std::make_shared<ServiceTerminationDetector>(
                this->hostname, action_executor, this->commport, true, false);
            failure_detector->setSimulation(this->simulation_);
            failure_detector->start(failure_detector, true, false);// Daemonized, no auto-restart

            // Keep track of this action executor
            this->action_executors[action] = action_executor;

            // Update core and RAM availability
            this->ram_availabilities[target_host] -= required_ram;
            this->running_thread_counts[target_host] += target_num_cores;

            dispatched_actions.insert(action);
        }

        // Remove the Actions from the ready queue (this is inefficient, better data structs would help)
        while (not dispatched_actions.empty()) {
            std::shared_ptr<Action> dispatched_action = *(dispatched_actions.begin());
            for (auto const &ready_action: this->ready_actions) {
                for (auto it = this->ready_actions.begin(); it != this->ready_actions.end(); it++) {
                    if ((*it) == dispatched_action) {
                        this->ready_actions.erase(it);
                        dispatched_actions.erase(dispatched_action);
                        break;
                    }
                }
            }
        }

        this->releaseDaemonLock();
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     */
    bool ActionExecutionService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        WRENCH_INFO("Got a [%s] message", message->getName().c_str());
        if (std::dynamic_pointer_cast<HostHasTurnedOnMessage>(message)) {
            this->num_hosts_turned_on++;
            // Do nothing, just wake up
            return true;

        } else if (std::dynamic_pointer_cast<HostHasChangedSpeedMessage>(message)) {
            // Do nothing, just wake up
            return true;

        } else if (std::dynamic_pointer_cast<HostHasTurnedOffMessage>(message)) {
            this->num_hosts_turned_on--;
            if (this->getPropertyValueAsString(
                        ActionExecutionServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN) == "false") {
                return true;

            } else {
                // If not all resources are down or somebody is still running, nevermind
                // we may have gotten this "Host down" message before the "This Action Executor has crashed" message.
                // So we don't want to just quit right now. We'll
                //  get an Action Executor Crash message, at which point we'll check whether all hosts are down again
                if (not this->areAllComputeResourcesDownWithNoActionExecutorRunning()) {
                    WRENCH_INFO("Not terminating as there are still non-down resources and/or WUE executors that "
                                "haven't reported back yet");
                    return true;
                } else {
                    this->terminate(false, ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED);
                    this->exit_code = 1;// Exit code to signify that this is, in essence a crash (in case somebody cares)
                    return false;
                }
            }

        } else if (auto ssdm = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->terminate(ssdm->send_failure_notifications, (ComputeService::TerminationCause)(ssdm->termination_cause));

            // This is Synchronous
            try {
                ssdm->ack_commport->putMessage(
                        new ServiceDaemonStoppedMessage(0.0));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;

        } else if (auto aessarm = std::dynamic_pointer_cast<ActionExecutionServiceSubmitActionRequestMessage>(message)) {
            processSubmitAction(aessarm->reply_commport, aessarm->action);
            return true;

        } else if (auto aestarm = std::dynamic_pointer_cast<ActionExecutionServiceTerminateActionRequestMessage>(message)) {
            processActionTerminationRequest(aestarm->action, aestarm->reply_commport, aestarm->termination_cause);
            return true;

        } else if (auto aedm = std::dynamic_pointer_cast<ActionExecutorDoneMessage>(message)) {
            if (aedm->action_executor->getAction()->getState() == Action::State::COMPLETED) {
                processActionExecutorCompletion(aedm->action_executor);
            } else {
                processActionExecutorFailure(aedm->action_executor);
            }
            return true;

        } else if (auto shcm = std::dynamic_pointer_cast<ServiceHasCrashedMessage>(message)) {
            auto service = shcm->service;
            auto action_executor = std::dynamic_pointer_cast<ActionExecutor>(service);
            if (not action_executor) {
                throw std::runtime_error(
                        "Internal Error: Received a FailureDetectorServiceHasFailedMessage message, but that service is not "
                        "an ActionExecutor!");
            }
            processActionExecutorCrash(action_executor);
            // If all hosts being off should not cause the service to terminate, then nevermind
            if (this->getPropertyValueAsString(
                        ActionExecutionServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN) == "false") {
                return true;

            } else {
                // If not all resources are down or somebody is still running, nevermind
                // we may have gotten the "Host down" message before the "ActionExecutor Has Crashed" message.
                // So we don't want to just quit right now. We'll
                //  get an "ActionExecutor Has Crashed" message, at which point we'll check whether all hosts are down again
                if (not this->areAllComputeResourcesDownWithNoActionExecutorRunning()) {
                    return true;
                }

                WRENCH_INFO("ALL MY RESOURCES ARE DOWN");
                this->terminate(false, ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED);
                this->exit_code = 1;// Exit code to signify that this is, in essence a crash (in case somebody cares)
                return false;
            }

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief kill a running action
     * @param action: the action
     * @param cause: the failure cause
     */
    void ActionExecutionService::killAction(
            const std::shared_ptr<Action> &action,
            const std::shared_ptr<FailureCause> &cause) {
        WRENCH_INFO("Killing action %s", action->getName().c_str());

        bool killed_due_to_job_cancellation = (std::dynamic_pointer_cast<JobKilled>(cause) != nullptr);

        // If action is ready, remove it from ready list
        if (std::find(this->ready_actions.begin(), this->ready_actions.end(), action) != this->ready_actions.end()) {
            this->ready_actions.erase(std::find(this->ready_actions.begin(), this->ready_actions.end(), action));
        }

        // If action is running kill the executor
        if (this->action_executors.find(action) != this->action_executors.end()) {
            auto executor = this->action_executors[action];
            this->ram_availabilities[executor->getHost()] += executor->getMemoryAllocated();
            this->running_thread_counts[executor->getHost()] -= executor->getNumCoresAllocated();

            executor->kill(killed_due_to_job_cancellation);
            executor->getAction()->setFailureCause(cause);
            this->action_executors.erase(action);

            /** Yield, so that the executor has a chance to do their cleanup, etc. */
            S4U_Simulation::yield();
        }

        // Remove the action from the set of known actions
        this->all_actions.erase(action);
        this->action_run_specs.erase(action);

        // Set the action's state
        action->setState(Action::State::KILLED);
        action->setFailureCause(cause);

        // Send back an action failed message if necessary
        if (not killed_due_to_job_cancellation) {
            WRENCH_INFO("Sending action failure notification to '%s'",
                        this->parent_service->commport->get_cname());
            // NOTE: This is synchronous so that the process doesn't fall off the end
            try {
                this->parent_service->commport->dputMessage(
                        new ActionExecutionServiceActionDoneMessage(action, 0));
            } catch (ExecutionException &e) {
                return;
            }
        }
    }

    /**
     * @brief Terminate the daemon, dealing with pending/running actions
     */
    void ActionExecutionService::terminate(bool send_failure_notifications, ComputeService::TerminationCause termination_cause) {
        this->setStateToDown();

        WRENCH_INFO("Failing currently running actions");
        for (auto const &action: this->running_actions) {
            std::shared_ptr<FailureCause> failure_cause;
            switch (termination_cause) {
                case ComputeService::TerminationCause::TERMINATION_JOB_KILLED:
                    failure_cause = std::make_shared<JobKilled>(action->getJob());
                    break;
                case ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED:
                    failure_cause = std::make_shared<ServiceIsDown>(this->parent_service);
                    break;
                case ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT:
                    failure_cause = std::make_shared<JobTimeout>(action->getJob());
                    break;
                default:
                    failure_cause = std::make_shared<JobKilled>(action->getJob());
                    break;
            }
            this->killAction(action, failure_cause);
        }

        if (send_failure_notifications) {
            throw std::runtime_error("ActionExecutionService::terminate(): NEED TO IMPLEMENT FAILURE NOTIFICATIONS???");
        }
    }

    /**
     * @brief Synchronously terminate an action
     *
     * @param action: an action
     * @param termination_cause: termination cause
     *
     */
    void ActionExecutionService::terminateAction(std::shared_ptr<Action> action,
                                                 ComputeService::TerminationCause termination_cause) {
        assertServiceIsUp();

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        //  send a "terminate action" message to the daemon's commport
        this->commport->putMessage(
                new ActionExecutionServiceTerminateActionRequestMessage(
                        answer_commport, std::move(action), termination_cause, 0.0));

        // Get the answer
        auto msg = answer_commport->getMessage<ActionExecutionServiceTerminateActionAnswerMessage>(
                this->network_timeout,
                "ActionExecutionService::terminateAction(): Received an");
        // If no success, throw an exception
        if (not msg->success) {
            throw ExecutionException(msg->cause);
        }
    }

    /**
     * @brief Process an action executor completion
     * @param executor: the action executor
     */
    void ActionExecutionService::processActionExecutorCompletion(
            const std::shared_ptr<ActionExecutor> &executor) {

        auto executor_host = executor->getHost();
        auto action = executor->getAction();

        // Update RAM availabilities and running thread counts
        this->ram_availabilities[executor_host] += executor->getMemoryAllocated();
        this->running_thread_counts[executor_host] -= executor->getNumCoresAllocated();

        // Forget the action executor
        this->action_executors.erase(action);
        this->all_actions.erase(action);
        this->action_run_specs.erase(action);

        // Send the notification to the originator
        this->parent_service->commport->dputMessage(
                new ActionExecutionServiceActionDoneMessage(
                        action, 0.0));
    }

    /**
     * @brief Process an action executor failure
     * @param executor: the action executor
     */
    void ActionExecutionService::processActionExecutorFailure(const std::shared_ptr<ActionExecutor> &executor) {
        auto action = executor->getAction();
        auto cause = action->getFailureCause();

        // Update RAM availabilities and running thread counts
        this->ram_availabilities[executor->getHost()] += executor->getMemoryAllocated();
        this->running_thread_counts[executor->getHost()] -= executor->getNumCoresAllocated();

        // Forget the action executor
        this->action_executors.erase(action);

        // Send the notification
        WRENCH_INFO("Sending action failure notification to '%s'", parent_service->commport->get_cname());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
            auto msg = new ActionExecutionServiceActionDoneMessage(action, 0);
            this->parent_service->commport->dputMessage(msg);
        } catch (ExecutionException &) {
            return;
        }
    }

    /**
     * @brief Process an action termination request
     *
     * @param action: the action to terminate
     * @param answer_commport: the commport to which the answer message should be sent
     * @param termination_cause: the termination cause
     */
    void ActionExecutionService::processActionTerminationRequest(const std::shared_ptr<Action> &action,
                                                                 S4U_CommPort *answer_commport,
                                                                 ComputeService::TerminationCause termination_cause) {
        // If the action doesn't exit, we reply right away
        if (this->all_actions.find(action) == this->all_actions.end()) {
            WRENCH_INFO(
                    "Trying to terminate an action that's not (no longer?) running!");
            std::string error_message = "Action cannot be terminated because it is not running";
            auto answer_message = new ActionExecutionServiceTerminateActionAnswerMessage(
                    false,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ActionExecutionService>(), error_message),
                    0.0);
            answer_commport->dputMessage(answer_message);
            return;
        }

        std::shared_ptr<FailureCause> failure_cause;
        switch (termination_cause) {
            case ComputeService::TerminationCause::TERMINATION_JOB_KILLED:
                failure_cause = std::make_shared<JobKilled>(action->getJob());
                break;
            case ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED:
                failure_cause = std::make_shared<ServiceIsDown>(this->parent_service);
                break;
            case ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT:
                failure_cause = std::make_shared<JobTimeout>(action->getJob());
                break;
            default:
                failure_cause = std::make_shared<JobKilled>(action->getJob());
                break;
        }
        this->killAction(action, failure_cause);

        // reply
        auto answer_message = new ActionExecutionServiceTerminateActionAnswerMessage(
                true, nullptr, 0);
        answer_commport->dputMessage(answer_message);
    }

    /**
     * @brief Helper function that determines whether there is at least one host with
     *        some number of cores (or more) and some ram capacity (or more)
     * @param num_cores: number of cores
     * @param ram: ram capacity
     * @return true is a host was found
     */
    bool ActionExecutionService::isThereAtLeastOneHostWithResources(unsigned long num_cores, sg_size_t ram) const {
        for (auto const &r: this->compute_resources) {
            if ((std::get<0>(r.second) >= num_cores) and (std::get<1>(r.second) >= ram)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Helper method that determines whether a submitted action (with service-specific
     *        arguments) can run given available  resources
     *
     * @param action: the action
     * @return true if the action can run
     */
    bool ActionExecutionService::actionCanRun(const std::shared_ptr<Action> &action) {
        auto service_specific_arguments = action->getJob()->getServiceSpecificArguments();

        // No service-specific argument
        if ((service_specific_arguments.find(action->getName()) == service_specific_arguments.end()) or
            (service_specific_arguments[action->getName()].empty())) {
            if (not isThereAtLeastOneHostWithResources(action->getMinNumCores(), action->getMinRAMFootprint())) {
                return false;
            }
        }

        // Parse the service-specific argument
        std::tuple<simgrid::s4u::Host *, unsigned long> parsed_spec = parseResourceSpec(
                service_specific_arguments[action->getName()]);
        simgrid::s4u::Host *desired_host = std::get<0>(parsed_spec);
        unsigned long desired_num_cores = std::get<1>(parsed_spec);

        if (desired_host == nullptr) {
            // At this point the desired num cores in non-zero
            if (not isThereAtLeastOneHostWithResources(desired_num_cores, action->getMinRAMFootprint())) {
                return false;
            } else {
                return true;
            }
        }

        // At this point the host is not empty
        unsigned long minimum_required_num_cores;
        if (desired_num_cores == 0) {
            minimum_required_num_cores = action->getMinNumCores();
        } else {
            minimum_required_num_cores = desired_num_cores;
        }
        unsigned long num_cores_on_desired_host = std::get<0>(this->compute_resources[desired_host]);
        sg_size_t ram_on_desired_host = std::get<1>(this->compute_resources[desired_host]);

        if ((num_cores_on_desired_host < minimum_required_num_cores) or
            (ram_on_desired_host < action->getMinRAMFootprint())) {
            return false;
        }

        return true;
    }

    /**
     * @brief Process a submit action request
     *
     * @param answer_commport: the commport to which the answer message should be sent
     * @param action: the action
     *
     */
    void ActionExecutionService::processSubmitAction(
            S4U_CommPort *answer_commport, const std::shared_ptr<Action> &action) {
        WRENCH_INFO("Asked to run action %s", action->getName().c_str());

        auto service_specific_arguments = action->getJob()->getServiceSpecificArguments();

        // Can we run this action at all in terms of available resources?
        if (not actionCanRun(action)) {
            answer_commport->dputMessage(
                    new ActionExecutionServiceSubmitActionAnswerMessage(
                            false,
                            std::make_shared<NotEnoughResources>(action->getJob(), this->parent_service),
                            0.0));
            return;
        }

        // Construct the action run spec (i.e., keep track of service-specific arguments for the action)
        std::tuple<simgrid::s4u::Host *, unsigned long> action_run_spec;
        if ((service_specific_arguments.find(action->getName()) == service_specific_arguments.end()) or
            (service_specific_arguments[action->getName()].empty())) {
            action_run_spec = std::make_tuple(nullptr, 0);
        } else {
            std::string spec = service_specific_arguments[action->getName()];
            action_run_spec = parseResourceSpec(spec);
        }

        // We can now admit the action!
        this->all_actions.insert(action);
        this->action_run_specs[action] = action_run_spec;

        if (action->getState() == Action::State::READY)
            this->ready_actions.push_back(action);

        // And send a reply!
        answer_commport->dputMessage(
                new ActionExecutionServiceSubmitActionAnswerMessage(true, nullptr, 0.0));
    }

    /**
     * @brief Determine whether there is at least one host with (currently) available resources
     * @param num_cores: the desired number of cores
     * @param ram: the desired RAM
     *
     * @return true or false
     */
    bool ActionExecutionService::IsThereAtLeastOneHostWithAvailableResources(unsigned long num_cores,
                                                                             sg_size_t ram) {
        bool enough_ram = false;
        bool enough_cores = false;

        // First check RAM
        for (auto const &r: this->ram_availabilities) {
            if (r.second >= ram) {
                enough_ram = true;
                break;
            }
        }

        // Then check Cores
        if (enough_ram) {
            for (auto const &r: this->running_thread_counts) {
                unsigned long cores = std::get<0>(this->compute_resources[r.first]);
                unsigned long running_threads = r.second;
                if (running_threads > cores) {
                    throw std::runtime_error("ActionExecutionService::IsThereAtLeastOneHostWithAvailableResources(): The number of running threads exceeds "
                                             "the number of cores. This should not have happened [Internal error]");
                }
                if (running_threads < cores and (cores - running_threads >= num_cores)) {
                    enough_cores = true;
                    break;
                }
            }
        }

        return enough_ram and enough_cores;
    }

    /**
     * @brief Return resource information
     * @param key: the information key
     * @return resource information
     */
    std::map<std::string, double> ActionExecutionService::getResourceInformation(const std::string &key) {
        // Build a dictionary
        std::map<std::string, double> information;

        if (key == "num_hosts") {
            // Num hosts
            std::map<std::string, double> num_hosts;
            num_hosts.insert(std::make_pair(this->getName(), this->compute_resources.size()));
            return num_hosts;

        } else if (key == "num_cores") {
            // Num cores per hosts
            std::map<std::string, double> num_cores;
            for (auto const &r: this->compute_resources) {
                num_cores.insert(std::make_pair(r.first->get_name(), static_cast<double>(std::get<0>(r.second))));
            }
            return num_cores;

        } else if (key == "num_idle_cores") {
            // Num idle cores per hosts
            std::map<std::string, double> num_idle_cores;
            for (auto const &r: this->running_thread_counts) {
                unsigned long cores = std::get<0>(this->compute_resources[r.first]);
                const unsigned long running_threads = r.second;
                num_idle_cores.insert(
                        std::make_pair(r.first->get_name(), static_cast<double>(std::max<unsigned long>(cores - running_threads, 0))));
            }
            return num_idle_cores;

        } else if (key == "flop_rates") {
            // Flop rate per host
            std::map<std::string, double> flop_rates;
            for (auto h: this->compute_resources) {
                flop_rates.insert(std::make_pair(h.first->get_name(), std::get<0>(h)->get_speed()));
            }
            return flop_rates;

        } else if (key == "ram_capacities") {
            // RAM capacity per host
            std::map<std::string, double> ram_capacities;
            for (auto h: this->compute_resources) {
                ram_capacities.insert(std::make_pair(h.first->get_name(), S4U_Simulation::getHostMemoryCapacity(std::get<0>(h))));
            }
            return ram_capacities;

        } else if (key == "ram_availabilities") {
            // RAM availability per host
            std::map<std::string, double> ram_availabilities_to_return;
            for (auto const &r: this->ram_availabilities) {
                ram_availabilities_to_return.insert(std::make_pair(r.first->get_name(), r.second));
            }
            return ram_availabilities_to_return;

        } else {
            throw std::runtime_error("ActionExecutionService::getResourceInformation(): unknown key");
        }
    }

    /**
     * @brief Method to make sure that property specs are valid
     *
     */
    void ActionExecutionService::validateProperties() {
        // bool success = true;

        // if (not success) {
        //     throw std::invalid_argument("ActionExecutionService: Invalid properties");
        // }
    }

    /**
     * @brief Process a crash of an ActionExecutor (although some work may has been done, we'll just
     *        re-do the action from scratch)
     *
     * @param executor: the action executor that has crashed
     */
    void ActionExecutionService::processActionExecutorCrash(const std::shared_ptr<ActionExecutor> &executor) {
        std::shared_ptr<Action> action = executor->getAction();

        WRENCH_INFO("Handling an ActionExecutor crash!");

        // Update RAM availabilities and running thread counts
        this->ram_availabilities[executor->getHost()] += executor->getMemoryAllocated();
        this->running_thread_counts[executor->getHost()] -= executor->getNumCoresAllocated();

        // Forget the executor
        this->action_executors.erase(action);

        if (not this->getPropertyValueAsBoolean(ActionExecutionServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH)) {
            // Reset the action state to READY
            action->newExecution(Action::State::READY);
            //            action->setState(Action::State::READY);
            // Put the action back in the ready list (at the end)
            WRENCH_INFO("Putting action %s back in the ready queue", action->getName().c_str());
            this->ready_actions.push_back(action);
        } else {
            // Send the notification
            WRENCH_INFO("Sending action failure notification to '%s'", parent_service->commport->get_cname());
            // NOTE: This is synchronous so that the process doesn't fall off the end
            try {
                this->parent_service->commport->dputMessage(
                        new ActionExecutionServiceActionDoneMessage(action, 0));
            } catch (ExecutionException &) {
                return;
            }
        }
    }

    /**
     * @brief A helper method to checks if all compute resources are down
     * @return true or false
     */
    bool ActionExecutionService::areAllComputeResourcesDownWithNoActionExecutorRunning() const {
        bool all_resources_down = true;
        //        for (auto const &h: this->compute_resources) {
        //            if (h.first->is_on()) {
        //                all_resources_down = false;
        //                break;
        //            }
        //        }
        all_resources_down = (this->num_hosts_turned_on == 0);

        return all_resources_down and (this->action_executors.empty());
    }

    /**
     * @brief Set parent service
     * @param parent: the parent service
     */
    void ActionExecutionService::setParentService(std::shared_ptr<Service> parent) {
        this->parent_service = std::move(parent);
    }

    /**
     * @brief Get a (reference to) the compute resources of this service
     * @return the compute resources
     */
    std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> &ActionExecutionService::getComputeResources() {
        return this->compute_resources;
    }

    /**
     * @brief Get the parent compute service (could be nullptr if stand-alone)
     * @return a compute service
     */
    std::shared_ptr<Service> ActionExecutionService::getParentService() const {
        return this->parent_service;
    }
}// namespace wrench
