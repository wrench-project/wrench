/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <typeinfo>
#include <map>
#include <memory>
#include <wrench/util/PointerUtil.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/helpers/HostStateChangeDetectorMessage.h>


#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "helper_services/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/services/helpers/ServiceTerminationDetectorMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/services/helpers/Alarm.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/services/helpers/ServiceTerminationDetector.h"
#include "wrench/services/helpers/HostStateChangeDetector.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(bare_metal_compute_service, "Log category for BareMetalComputeService");


namespace wrench {

    /**
     * @brief Destructor
     */
    BareMetalComputeService::~BareMetalComputeService() {
        this->default_property_values.clear();
    }


    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void BareMetalComputeService::cleanup(bool has_returned_from_main, int return_value) {

        // Do the default behavior (which will throw as this is not a fault-tolerant service)
        Service::cleanup(has_returned_from_main, return_value);

        // Clean up state in case of a restart
        for (auto host : this->compute_resources) {
            this->total_num_cores += std::get<0>(host.second);
            this->ram_availabilities.insert(
                    std::make_pair(host.first, S4U_Simulation::getHostMemoryCapacity(host.first)));
            this->running_thread_counts.insert(std::make_pair(host.first, 0));
        }

        this->running_jobs.clear();
        this->job_run_specs.clear();
        this->all_workunits.clear();
        this->ready_workunits.clear();
        this->completed_workunits.clear();
        this->workunit_executors.clear();

    }

    /**
     * @brief Helper static method to parse resource specifications to the <cores,ram> format
     * @param spec: specification string
     * @return a <cores, ram> tuple
     * @throw std::invalid_argument
     */
    static std::tuple<std::string, unsigned long> parseResourceSpec(std::string spec) {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, spec, boost::is_any_of(":"));
        switch (tokens.size()) {
            case 1: // "num_cores" or "hostname"
            {
                unsigned long num_threads;
                if (sscanf(tokens[0].c_str(), "%lu", &num_threads) != 1) {
                    return std::make_tuple(tokens[0], 0);
                } else {
                    return std::make_tuple(std::string(""), num_threads);
                }
            }
            case 2: // "hostname:num_cores"
            {
                unsigned long num_threads;
                if (sscanf(tokens[1].c_str(), "%lu", &num_threads) != 1) {
                    throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
                }
                return std::make_tuple(tokens[0], num_threads);
            }
            default: {
                throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
            }
        }
    }


    /**
     * @brief Submit a standard job to the compute service
     * @param job: a standard job
     * @param service_specific_args: optional service specific arguments
     *
     *    These arguments are provided as a map of strings, indexed by task IDs. These
     *    strings are formatted as "[hostname:][num_cores]" (e.g., "somehost:12", "somehost","6", "").
     *
     *      - If a value is not provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *      - If a "" value is provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *      - If a "hostname" value is provided for a task, then the service will run the task on that
     *        host, using as many of its cores as possible
     *      - If a "num_cores" value is provided for a task, then the service will run that task with
     *        this many cores, but will choose the host on which to run it.
     *      - If a "hostname:num_cores" value is provided for a task, then the service will run that
     *        task with this many cores on that host.
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void BareMetalComputeService::submitStandardJob(StandardJob *job,
                                                    std::map<std::string, std::string> &service_specific_args) {

        assertServiceIsUp();

        /* make sure that service arguments are provided for tasks in the jobs */
        for (auto const &arg : service_specific_args) {
            bool found = false;
            for (auto const &task : job->getTasks()) {
                if (task->getID() == arg.first) {
                    found = true;
                    break;
                }
            }
            if (not found) {
                throw std::invalid_argument(
                        "BareMetalComputeService::submitStandardJob(): Service-specific argument provided for task with ID '" +
                        arg.first + "' but there is no task with such ID in the job");
            }
        }

        // Check that service-specific args that are provided are well-formatted
        for (auto t : job->getTasks()) {

            if ((service_specific_args.find(t->getID()) != service_specific_args.end()) and
                (not service_specific_args[t->getID()].empty())) {
                std::tuple<std::string, unsigned long> parsed_spec;

                try {
                    parsed_spec = parseResourceSpec(service_specific_args[t->getID()]);
                } catch (std::invalid_argument &e) {
                    throw;
                }

                std::string target_host = std::get<0>(parsed_spec);
                unsigned long target_num_cores = std::get<1>(parsed_spec);


                if (not target_host.empty()) {
                    if (this->compute_resources.find(target_host) == this->compute_resources.end()) {
                        throw std::invalid_argument(
                                "Invalid service-specific argument '" + service_specific_args[t->getID()] +
                                "' for task '" +
                                t->getID() + "': no such host");
                    }
                }

                if (target_num_cores > 0) {
                    if (target_num_cores < t->getMinNumCores()) {
                        throw std::invalid_argument(
                                "Invalid service-specific argument '" + service_specific_args[t->getID()] +
                                "' for task '" +
                                t->getID() + "': the task requires at least " + std::to_string(t->getMinNumCores()) +
                                " cores");
                    }
                    if (target_num_cores > t->getMaxNumCores()) {
                        throw std::invalid_argument(
                                "Invalid service-specific argument '" + service_specific_args[t->getID()] +
                                "' for task '" +
                                t->getID() + "': the task can use at most " + std::to_string(t->getMaxNumCores()) +
                                " cores");
                    }
                }
            }
        }

        // At this point, there may still be insufficient resources to run the task, but that will
        // be handled later (and a WorkflowExecutionError with a "not enough resources" FailureCause
        // may be generated).

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

        //  send a "run a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new ComputeServiceSubmitStandardJobRequestMessage(
                                            answer_mailbox, job, service_specific_args,
                                            this->getMessagePayloadValue(
                                                    ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitStandardJobAnswerMessage>(message)) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "ComputeService::submitStandardJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }

    /**
     * @brief Asynchronously submit a pilot job to the compute service. This will raise
     *        a WorkflowExecutionException as this service does not support pilot jobs.
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments (only {} is supported)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void
    BareMetalComputeService::submitPilotJob(PilotJob *job,
                                            std::map<std::string, std::string> &service_specific_args) {

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

        // Send a "run a pilot job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceSubmitPilotJobRequestMessage(
                            answer_mailbox, job, service_specific_args, this->getMessagePayloadValue(
                                    BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitPilotJobAnswerMessage>(message)) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            } else {
                return;
            }

        } else {
            throw std::runtime_error(
                    "BareMetalComputeService::submitPilotJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a map of <num_cores, memory> tuples, indexed by hostname, which represents
     *        the compute resources available to this service.
     *          - use num_cores = ComputeService::ALL_CORES to use all cores available on the host
     *          - use memory = ComputeService::ALL_RAM to use all RAM available on the host
     * @param scratch_space_mount_point: the compute service's scratch space's mount point ("" means none)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    BareMetalComputeService::BareMetalComputeService(
            const std::string &hostname,
            const std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::string scratch_space_mount_point,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list
    ) :
            ComputeService(hostname,
                           "bare_metal",
                           "bare_metal",
                           scratch_space_mount_point) {


        initiateInstance(hostname,
                         std::move(compute_resources),
                         std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_hosts:: the names of the hosts available as compute resources (the service
     *        will use all the cores and all the RAM of each host)
     * @param scratch_space_mount_point: the compute service's scratch space's mount point ("" means none)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    BareMetalComputeService::BareMetalComputeService(const std::string &hostname,
                                                     const std::vector<std::string> compute_hosts,
                                                     std::string scratch_space_mount_point,
                                                     std::map<std::string, std::string> property_list,
                                                     std::map<std::string, double> messagepayload_list
    ) :
            ComputeService(hostname,
                           "bare_metal",
                           "bare_metal",
                           scratch_space_mount_point) {

        std::map<std::string, std::tuple<unsigned long, double>> specified_compute_resources;
        for (auto h : compute_hosts) {
            specified_compute_resources.insert(
                    std::make_pair(h, std::make_tuple(ComputeService::ALL_CORES, ComputeService::ALL_RAM)));
        }

        initiateInstance(hostname,
                         specified_compute_resources,
                         std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);
    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a list of <hostname, num_cores, memory> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param ttl: the time-to-live, in seconds (DBL_MAX: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    BareMetalComputeService::BareMetalComputeService(
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            double ttl,
            PilotJob *pj,
            std::string suffix, std::shared_ptr<StorageService> scratch_space) : ComputeService(hostname,
                                                                                                "bare_metal" + suffix,
                                                                                                "bare_metal" + suffix,
                                                                                                scratch_space) {

        initiateInstance(hostname,
                         std::move(compute_resources),
                         std::move(property_list),
                         std::move(messagepayload_list),
                         ttl,
                         pj);
    }


    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param compute_hosts:: a list of <hostname, num_cores, memory> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param scratch_space: the scratch space for this compute service
     */
    BareMetalComputeService::BareMetalComputeService(const std::string &hostname,
                                                     const std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                                     std::map<std::string, std::string> property_list,
                                                     std::map<std::string, double> messagepayload_list,
                                                     std::shared_ptr<StorageService> scratch_space) :
            ComputeService(hostname,
                           "bare_metal",
                           "bare_metal",
                           scratch_space) {

        initiateInstance(hostname,
                         compute_resources,
                         std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);

    }


    /**
     * @brief Helper method called by all constructors to initiate object instance
     *
     * @param hostname: the name of the host
     * @param compute_resources: compute_resources: a map of <num_cores, memory> pairs, indexed by hostname, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param ttl: the time-to-live, in seconds (DBL_MAX: infinite time-to-live)
     * @param pj: a containing PilotJob  (nullptr if none)
     *
     * @throw std::invalid_argument
     */
    void BareMetalComputeService::initiateInstance(
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            double ttl,
            PilotJob *pj) {

        if (ttl < 0) {
            throw std::invalid_argument(
                    "BareMetalComputeService::initiateInstance(): invalid TTL value (must be >0)");
        }

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Validate that properties are correct
        this->validateProperties();

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        // Check that there is at least one core per host and that hosts have enough cores
        for (auto host : compute_resources) {

            std::string hname = host.first;
            unsigned long requested_cores = std::get<0>(host.second);
            unsigned long available_cores;
            try {
                available_cores = S4U_Simulation::getHostNumCores(hname);
            } catch (std::runtime_error &e) {
                throw std::invalid_argument(
                        "BareMetalComputeService::initiateInstance(): Host '" + hname + "' does not exist");
            }
            if (requested_cores == ComputeService::ALL_CORES) {
                requested_cores = available_cores;
            }
            if (requested_cores == 0) {
                throw std::invalid_argument(
                        "BareMetalComputeService::BareMetalComputeService(): at least 1 core should be requested");
            }
            if (requested_cores > available_cores) {
                throw std::invalid_argument(
                        "BareMetalComputeService::BareMetalComputeService(): host " + hname + "only has " +
                        std::to_string(available_cores) + " cores but " +
                        std::to_string(requested_cores) + " are requested");
            }

            double requested_ram = std::get<1>(host.second);
            double available_ram = S4U_Simulation::getHostMemoryCapacity(hname);
            if (requested_ram < 0) {
                throw std::invalid_argument(
                        "BareMetalComputeService::BareMetalComputeService(): requested ram should be non-negative");
            }

            if (requested_ram == ComputeService::ALL_RAM) {
                requested_ram = available_ram;
            }

            if (requested_ram > available_ram) {
                throw std::invalid_argument(
                        "BareMetalComputeService::BareMetalComputeService(): host " + hname + "only has " +
                        std::to_string(available_ram) + " bytes of RAM but " +
                        std::to_string(requested_ram) + " are requested");
            }

            this->compute_resources.insert(std::make_pair(hname, std::make_tuple(requested_cores, requested_ram)));
        }

        // Compute the total number of cores and set initial ram availabilities
        this->total_num_cores = 0;
        for (auto host : this->compute_resources) {
            this->total_num_cores += std::get<0>(host.second);
            this->ram_availabilities.insert(
                    std::make_pair(host.first, S4U_Simulation::getHostMemoryCapacity(host.first)));
            this->running_thread_counts.insert(std::make_pair(host.first, 0));
        }


        this->ttl = ttl;
        this->has_ttl = (this->ttl != DBL_MAX);
        this->containing_pilot_job = pj;

    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BareMetalComputeService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO("New BareMetal Compute Service starting (%s) on %ld hosts with a total of %ld cores",
                    this->mailbox_name.c_str(), this->compute_resources.size(), this->total_num_cores);

        {
            // Create the host state monitor
            std::vector<std::string> hosts_to_monitor;
            for (auto const &h : this->compute_resources) {
                hosts_to_monitor.push_back(h.first);
            }
            this->host_state_change_monitor = std::shared_ptr<HostStateChangeDetector>(
                    new HostStateChangeDetector(this->hostname, hosts_to_monitor, true, true, true,
                                                this->getSharedPtr<Service>(), this->mailbox_name,
                                                {{HostStateChangeDetectorProperty::MONITORING_PERIOD, "1.0"}}));
            this->host_state_change_monitor->simulation = this->simulation;
            this->host_state_change_monitor->start(this->host_state_change_monitor, true,
                                                   false); // Daemonized, no auto-restart
        }

        // Set an alarm for my timely death, if necessary
        if (this->has_ttl) {
            this->death_date = S4U_Simulation::getClock() + this->ttl;
        }

        /** Main loop **/
        while (this->processNextMessage()) {

            /** Dispatch ready work units **/
            this->dispatchReadyWorkunits();

        }

        WRENCH_INFO("BareMetalComputeService on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return this->exit_code;
    }

    /**
     * @brief helper function to figure out where/how a task should run
     *
     * @param task: the workflow task for which this allocation is being computed (if nullptr: none)
     * @param required_host: the required host per service-specific arguments ("" means: choose one)
     * @param required_num_cores: the required number of cores per service-specific arguments (0 means: choose a number)
     * @param required_ram: the required number of bytes of RAM
     * @param hosts_to_avoid: a list of hosts to not even consider
     * @return
     */
    std::tuple<std::string, unsigned long> BareMetalComputeService::pickAllocation(WorkflowTask *task,
                                                                                   std::string required_host,
                                                                                   unsigned long required_num_cores,
                                                                                   double required_ram,
                                                                                   std::set<std::string> &hosts_to_avoid) {

        // Compute possible hosts
        std::set<std::string> possible_hosts;
        std::string new_host_to_avoid = "";
        double new_host_to_avoid_ram_capacity = 0;
        for (auto const &r : this->compute_resources) {


            // If there is a required host, then don't even look at others
            if (not required_host.empty() and (r.first != required_host)) {
                continue;
            }

            // If the host is down, then don't look at it
            if (not Simulation::isHostOn(r.first)) {
                continue;
            }

            // If the host has compute speed zero, then don't look at it
            if (Simulation::getHostFlopRate(r.first) <= 0.0) {
                continue;
            }

            if ((required_num_cores == 0) and (std::get<0>(r.second) < task->getMinNumCores())) {
                continue;
            }
            if ((required_num_cores != 0) and (std::get<0>(r.second) < required_num_cores)) {
                continue;
            }
            if ((required_ram > 0) and (hosts_to_avoid.find(r.first) != hosts_to_avoid.end())) {
                continue;
            }
            if ((required_ram > 0) and (this->ram_availabilities[r.first] < required_ram)) {
                if (new_host_to_avoid.empty()) {
                    new_host_to_avoid = r.first;
                    new_host_to_avoid_ram_capacity = this->ram_availabilities[r.first];
                } else {
                    if (this->ram_availabilities[r.first] > new_host_to_avoid_ram_capacity) {
                        // Make sure we "Avoid" the host with the most RAM (as it might becomes usable sooner
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
            if (not new_host_to_avoid.empty()) {
                hosts_to_avoid.insert(new_host_to_avoid);
            }
            return std::make_tuple(std::string(), 0);
        }

        // Select the "best" host
        double lowest_load = DBL_MAX;
        std::string picked_host = "";
        unsigned long picked_num_cores = 0;
        for (auto const &h : possible_hosts) {
            unsigned long num_running_threads = this->running_thread_counts[h];
            unsigned long num_cores = std::get<0>(this->compute_resources[h]);
            double flop_rate = S4U_Simulation::getHostFlopRate(h);
            unsigned long used_num_cores;
            if (required_num_cores == 0) {
                used_num_cores = std::min(num_cores, task->getMaxNumCores()); // as many cores as possible
            } else {
                used_num_cores = required_num_cores;
            }
            // A totally heuristic load estimate
            double load = ((double) ((num_running_threads + used_num_cores) / num_cores)) / flop_rate;
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
    void BareMetalComputeService::dispatchReadyWorkunits() {

        // Don't kill me while I am doing this
        this->acquireDaemonLock();

        std::set<std::shared_ptr<Workunit>> dispatched_wus_for_job;

        std::set<std::string> no_longer_considered_hosts;  // Due to a previously considered workunit not being
        // able to run on that host due to RAM, and because we don't
        // allow non-zero-ram tasks to jump ahead of other tasks

        for (auto const &wu : this->ready_workunits) {

            std::string picked_host;

            StandardJob *job = wu->getJob();
            std::string target_host;
            unsigned long target_num_cores;
            double required_ram;
            if (wu->task == nullptr) {
                // Always run on the first host
                std::tuple<std::string, unsigned long> allocation =
                        pickAllocation(nullptr, "", 1, 0.0, no_longer_considered_hosts);
                required_ram = 0.0;
                target_host = std::get<0>(allocation);
                target_num_cores = std::get<1>(allocation);
            } else {
                std::tuple<std::string, unsigned long> allocation =
                        pickAllocation(wu->task,
                                       std::get<0>(this->job_run_specs[job][wu->task]),
                                       std::get<1>(this->job_run_specs[job][wu->task]),
                                       wu->task->getMemoryRequirement(),
                                       no_longer_considered_hosts);
                required_ram = wu->task->getMemoryRequirement();
                target_host = std::get<0>(allocation);
                target_num_cores = std::get<1>(allocation);
            }

            // If we didn't find a host, forget it
            if (target_host.empty()) {
                WRENCH_INFO("NO DICE");
                continue;
            }

            /** Dispatch it **/
            // Create a workunit executor on the target host
            std::shared_ptr<WorkunitExecutor> workunit_executor = std::shared_ptr<WorkunitExecutor>(
                    new WorkunitExecutor(target_host,
                                         target_num_cores,
                                         required_ram,
                                         this->mailbox_name,
                                         wu,
                                         this->getScratch(),
                                         job,
                                         this->getPropertyValueAsDouble(
                                                 BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD),
                                         false
                    ));

            workunit_executor->simulation = this->simulation;
            try {
                workunit_executor->start(workunit_executor, true, false); // Daemonized, no auto-restart
            } catch (std::shared_ptr<HostError> &e) {
                // This is an error on the target host!!
                throw std::runtime_error(
                        "BareMetalComputeService::dispatchReadyWorkunits(): got a host error on the target host - this shouldn't happen");
            }


            // Start a failure detector for this workunit executor (which will send me a message in case the
            // work unit executor has died)
            auto failure_detector = std::shared_ptr<ServiceTerminationDetector>(
                    new ServiceTerminationDetector(this->hostname, workunit_executor, this->mailbox_name, true, false));
            failure_detector->simulation = this->simulation;
            failure_detector->start(failure_detector, true, false); // Daemonized, no auto-restart

            // Keep track of this workunit executor
            this->workunit_executors[job].insert(workunit_executor);

            // Update core and RAM availability
            this->ram_availabilities[target_host] -= required_ram;
            this->running_thread_counts[target_host] += target_num_cores;

            dispatched_wus_for_job.insert(wu);
        }

        // Remove the WUs from the ready queue (this is inefficient, better data structs would help)
        while (dispatched_wus_for_job.size() > 0) {
            std::shared_ptr<Workunit> wu = *(dispatched_wus_for_job.begin());
            for (auto it = this->ready_workunits.begin(); it != this->ready_workunits.end(); it++) {
                if ((*it) == wu) {
                    this->ready_workunits.erase(it);
                    dispatched_wus_for_job.erase(wu);
                    break;
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
     * @throw std::runtime_error
     */
    bool BareMetalComputeService::processNextMessage() {

        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &error) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());
        if (auto msg = std::dynamic_pointer_cast<HostHasTurnedOnMessage>(message)) {
            // Do nothing, just wake up
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<HostHasChangedSpeedMessage>(message)) {
            // Do nothing, just wake up
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<HostHasTurnedOffMessage>(message)) {
            // If all hosts being off should not cause the service to terminate, then nevermind
            if (this->getPropertyValueAsString(
                    BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN) == "false") {
                return true;
            } else {

                // If not all resources are down or somebody is still running, nevermind
                // we may have gotten the "Host down" message before the "This WUE has crashed" message.
                // So we don't want to just quit right now. We'll
                //  get a WUE Crash message, at which point we'll check whether all hosts are down again
                if (not this->areAllComputeResourcesDownWithNoWUERunning()) {
                    WRENCH_INFO(
                            "Not terminating as there are still non-down resources and/or WUE executors that haven't reported back yet");
                    return true;
                }

                this->terminate(true);
                this->exit_code = 1; // Exit code to signify that this is, in essence a crash (in case somebody cares)
                return false;
            }

        } else if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {

            this->terminate(false);

            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                BareMetalComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitStandardJobRequestMessage>(message)) {
            processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitPilotJobRequestMessage>(message)) {
            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceResourceInformationRequestMessage>(message)) {
            processGetResourceInformation(msg->answer_mailbox);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminateStandardJobRequestMessage>(message)) {
            processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<WorkunitExecutorDoneMessage>(message)) {
            processWorkunitExecutorCompletion(msg->workunit_executor, msg->workunit);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<WorkunitExecutorFailedMessage>(message)) {
            processWorkunitExecutorFailure(msg->workunit_executor, msg->workunit, msg->cause);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ServiceHasCrashedMessage>(message)) {
            auto service = msg->service;
            auto workunit_executor = std::dynamic_pointer_cast<WorkunitExecutor>(service);
            if (not workunit_executor) {
                throw std::runtime_error(
                        "Received a FailureDetectorServiceHasFailedMessage message, but that service is not a WorkUnitExecutor!");
            }
            processWorkunitExecutorCrash(workunit_executor);
            // If all hosts being off should not cause the service to terminate, then nevermind
            if (this->getPropertyValueAsString(
                    BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN) == "false") {
                return true;
            } else {

                // If not all resources are down or somebody is still running, nevermind
                // we may have gotten the "Host down" message before the "This WUE has crashed" message.
                // So we don't want to just quit right now. We'll
                //  get a WUE Crash message, at which point we'll check whether all hosts are down again
                if (not this->areAllComputeResourcesDownWithNoWUERunning()) {
                    return true;
                }

                this->terminate(true);
                this->exit_code = 1; // Exit code to signify that this is, in essence a crash (in case somebody cares)
                return false;
            }

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }


/**
 * @brief fail a running standard job
 * @param job: the job
 * @param cause: the failure cause
 */
    void
    BareMetalComputeService::failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {

        WRENCH_INFO("Failing running job %s", job->getName().c_str());

        terminateRunningStandardJob(job, BareMetalComputeService::JobTerminationCause::COMPUTE_SERVICE_KILLED);

        // Send back a job failed message (Not that it can be a partial fail)
        WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(
                                            job, this->getSharedPtr<BareMetalComputeService>(), cause,
                                            this->getMessagePayloadValue(
                                                    BareMetalComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }
    }

/**
* @brief terminate a running standard job
* @param job: the job
*/
    void BareMetalComputeService::terminateRunningStandardJob(StandardJob *job,
                                                              BareMetalComputeService::JobTerminationCause termination_cause) {

        /** Kill all relevant work unit executors */
        for (auto const &wue : this->workunit_executors[job]) {
            for (auto const &f : wue->getFilesStoredInScratch()) {
                this->files_in_scratch[job].insert(f);
            }
            if (wue->workunit->task) {
                this->ram_availabilities[wue->getHostname()] += wue->workunit->task->getMemoryRequirement();
                this->running_thread_counts[wue->getHostname()] -= wue->getNumCores();
            }
            wue->kill(termination_cause == BareMetalComputeService::JobTerminationCause::TERMINATED);
        }
        this->workunit_executors[job].clear();
        this->workunit_executors.erase(job);

        /** Yield, so that all working_executors have a chance to do their cleanup, etc. */
        S4U_Simulation::yield();

        /** Remove all relevant work units */
        std::set<std::shared_ptr<Workunit>> to_remove;
        for (auto const &wu : this->ready_workunits) {
            if (wu->getJob() == job) {
                to_remove.insert(wu);
            }
        }
        // Really inefficient, Better data structures needed
        while (not to_remove.empty()) {
            for (auto it = this->ready_workunits.begin(); it != this->ready_workunits.end(); it++) {
                if ((*it) == (*(to_remove.begin()))) {
                    this->ready_workunits.erase(it);
                    to_remove.erase(to_remove.begin());
                    break;
                }
            }
        }
        this->completed_workunits[job].clear();
        this->completed_workunits.erase(job);
        this->all_workunits[job].clear();
        this->all_workunits.erase(job);

        /** Deal with task states (note that simulation timestamps are set in the clean() function) */
        for (auto failed_task: job->getTasks()) {
            switch (failed_task->getInternalState()) {
                case WorkflowTask::InternalState::TASK_NOT_READY:
                case WorkflowTask::InternalState::TASK_READY:
                case WorkflowTask::InternalState::TASK_COMPLETED:
                    break;

                case WorkflowTask::InternalState::TASK_RUNNING:
                    throw std::runtime_error(
                            "BareMetalComputeService::terminateRunningStandardJob(): task state shouldn't be 'RUNNING'"
                            "after a WorkUnitExecutor was killed!");
                case WorkflowTask::InternalState::TASK_FAILED:
                    // Making failed task READY again!!!
                    failed_task->setInternalState(WorkflowTask::InternalState::TASK_READY);
                    break;

                default:
                    throw std::runtime_error(
                            "BareMetalComputeService::terminateRunningStandardJob(): unexpected task state");
            }
        }

        // Remove files from Scratch
        if (this->containing_pilot_job == nullptr) {
            for (auto const &f : this->files_in_scratch[job]) {
                try {
                    StorageService::deleteFile(f,
                                               FileLocation::LOCATION(this->getScratch(),
                                                                      this->getScratch()->getMountPoint() +
                                                                      "/" + job->getName()),
                                               nullptr);
                } catch (WorkflowExecutionException &e) {
                    // ignore (perhaps it was never written)
                }
            }
            this->files_in_scratch[job].clear();
            this->files_in_scratch.erase(job);
        }
    }


/**
* @brief Declare all current jobs as failed (likely because the daemon is being terminated
* or has timed out (because it's in fact a pilot job))
*/
    void BareMetalComputeService::failCurrentStandardJobs() {


        for (auto job : this->running_jobs) {
            this->failRunningStandardJob(job, std::shared_ptr<FailureCause>(
                    new JobKilled(job, this->getSharedPtr<BareMetalComputeService>())));
        }
    }


/**
 * @brief Terminate the daemon, dealing with pending/running jobs
 *
 * @param notify_pilot_job_submitters:
 */
    void BareMetalComputeService::terminate(bool notify_pilot_job_submitters) {

        this->setStateToDown();

        WRENCH_INFO("Failing current standard jobs");
        this->failCurrentStandardJobs();

        // At this point, nothing is running and no host is up
        if (this->containing_pilot_job != nullptr) {
            /*** Clean up everything in the scratch space ***/
            cleanUpScratch();
        }

        // Am I myself a pilot job?
        if (notify_pilot_job_submitters && this->containing_pilot_job) {

            WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox_name %s",
                        this->containing_pilot_job->getCallbackMailbox().c_str());
            // NOTE: This is synchronous so that the process doesn't fall off the end
            try {
                S4U_Mailbox::putMessage(this->containing_pilot_job->popCallbackMailbox(),
                                        new ComputeServicePilotJobExpiredMessage(
                                                this->containing_pilot_job,
                                                this->getSharedPtr<BareMetalComputeService>(),
                                                this->getMessagePayloadValue(
                                                        BareMetalComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
        }
    }


/**
 * @brief Synchronously terminate a standard job previously submitted to the compute service
 *
 * @param job: a standard job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 */
    void BareMetalComputeService::terminateStandardJob(StandardJob *job) {

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

        //  send a "terminate a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new ComputeServiceTerminateStandardJobRequestMessage(
                                            answer_mailbox, job, this->getMessagePayloadValue(
                                                    BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminateStandardJobAnswerMessage>(message)) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "BareMetalComputeService::terminateStandardJob(): Received an unexpected [" +
                    message->getName() + "] message!");
        }
    }


/**
 * @brief Process a workunit executor completion
 * @param workunit_executor: the workunit executor
 * @param workunit: the workunit
 */

    void BareMetalComputeService::processWorkunitExecutorCompletion(std::shared_ptr<WorkunitExecutor> workunit_executor,
                                                                    std::shared_ptr<Workunit> workunit) {
        StandardJob *job = workunit_executor->getJob();

        // Get the scratch files that executor may have generated
        for (auto &f : workunit_executor->getFilesStoredInScratch()) {
            if (this->files_in_scratch.find(job) == this->files_in_scratch.end()) {
                this->files_in_scratch.insert(std::make_pair(job, (std::set<WorkflowFile *>) {}));
            }
            this->files_in_scratch[job].insert(f);
        }

        // Update RAM availabilities and running thread counts
        this->ram_availabilities[workunit_executor->getHostname()] += workunit_executor->getMemoryUtilization();
        this->running_thread_counts[workunit_executor->getHostname()] -= workunit_executor->getNumCores();

        // Forget the workunit executor
        forgetWorkunitExecutor(workunit_executor);

        // Don't kill me while I am doing this
        this->acquireDaemonLock();

        // Process task completions, if any
        if (workunit->task != nullptr) {
            WRENCH_INFO("A workunit executor completed task %s (and its state is: %s)", workunit->task->getID().c_str(),
                        WorkflowTask::stateToString(workunit->task->getInternalState()).c_str());

            // Increase the "completed tasks" count of the job
            job->incrementNumCompletedTasks();
        }

        // Set the workunit as completed
        this->completed_workunits[job].insert(workunit);

        // Update workunit dependencies if any
        for (auto const &child : workunit->children) {
            child->num_pending_parents--;
            if (child->num_pending_parents == 0) {
                // Make sure the child's tasks ready (paranoid)
                if (child->task != nullptr) {
                    if (child->task->getInternalState() != WorkflowTask::InternalState::TASK_READY) {
                        throw std::runtime_error(
                                "BareMetalComputeService::processWorkunitExecutorCompletion(): Weird task state " +
                                std::to_string(child->task->getInternalState()) + " for task " +
                                child->task->getID());
                    }
                }
                // Move the workunit to ready
                this->ready_workunits.push_back(child);
            }
        }

        this->releaseDaemonLock();

        // If the job is not done, just return
        if (this->completed_workunits[job].size() != this->all_workunits[job].size()) {
            return;
        }

        // At this point, the job is done and we can get rid of workunits
        this->completed_workunits[job].clear();
        this->completed_workunits.erase(job);
        this->all_workunits[job].clear();
        this->all_workunits.erase(job);

        // Clean up run specs
        this->job_run_specs.erase(job);

        // If not in a pilot job, remove all files in scratch
        if (this->containing_pilot_job == nullptr) {
            for (auto const &f : this->files_in_scratch[job]) {
                StorageService::deleteFile(f,
                                           FileLocation::LOCATION(this->getScratch(),
                                                                  this->getScratch()->getMountPoint() + job->getName()),
                                           nullptr);
            }
            this->files_in_scratch[job].clear();
            this->files_in_scratch.erase(job);
        }

        this->running_jobs.erase(job);

        // Send the callback to the originator
        S4U_Mailbox::dputMessage(
                job->popCallbackMailbox(), new ComputeServiceStandardJobDoneMessage(
                        job, this->getSharedPtr<BareMetalComputeService>(), this->getMessagePayloadValue(
                                BareMetalComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));

    }


/**
* @brief Process a workunit executor failure
* @param workunit_executor: the workunit executor
* @param workunit: the workunit
* @param cause: the failure cause
*/

    void BareMetalComputeService::processWorkunitExecutorFailure(std::shared_ptr<WorkunitExecutor> workunit_executor,
                                                                 std::shared_ptr<Workunit> workunit,
                                                                 std::shared_ptr<FailureCause> cause) {
        StandardJob *job = workunit_executor->getJob();

        // Get the scratch files that executor may have generated
        for (auto &f : workunit_executor->getFilesStoredInScratch()) {
            if (this->files_in_scratch.find(job) == this->files_in_scratch.end()) {
                this->files_in_scratch.insert(std::make_pair(job, (std::set<WorkflowFile *>) {}));
            }
            this->files_in_scratch[job].insert(f);
        }
        // Update RAM availabilities and running thread counts
        if (workunit->task) {
            this->ram_availabilities[workunit_executor->getHostname()] += workunit->task->getMemoryRequirement();
            this->running_thread_counts[workunit_executor->getHostname()] -= workunit_executor->getNumCores();
        }
        // Forget the workunit executor
        forgetWorkunitExecutor(workunit_executor);

        // Fail the job
        this->failRunningStandardJob(job, std::move(cause));

        this->job_run_specs.erase(job);
        this->running_jobs.erase(job);
    }


    /**
     * @brief Helper function to "forget" a workunit executor (and free memory)
     * @param workunit_executor: the workunit executor
     */
    void BareMetalComputeService::forgetWorkunitExecutor(std::shared_ptr<WorkunitExecutor> workunit_executor) {

        StandardJob *job = workunit_executor->getJob();
        std::shared_ptr<WorkunitExecutor> found_it;
        for (auto const &wue : this->workunit_executors[job]) {
            if (wue == workunit_executor) {
                found_it = wue;
            }
        }
        if (found_it == nullptr) {
            throw std::runtime_error(
                    "BareMetalComputeService::processWorkunitExecutorCompletion(): Couldn't find workunit executor");
        }
        this->workunit_executors[job].erase(found_it);

    }


/**
 * @brief Process a standard job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void BareMetalComputeService::processStandardJobTerminationRequest(StandardJob *job,
                                                                       const std::string &answer_mailbox) {

        // If the job doesn't exit, we reply right away
        if (this->all_workunits.find(job) == this->all_workunits.end()) {
            WRENCH_INFO("Trying to terminate a standard job that's not (no longer?) running!");
            std::string msg = "Job cannot be terminated because it is not running";
            ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                    job, this->getSharedPtr<BareMetalComputeService>(), false,
                    std::shared_ptr<FailureCause>(new NotAllowed(this->getSharedPtr<BareMetalComputeService>(), msg)),
                    this->getMessagePayloadValue(
                            BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
            return;
        }

        terminateRunningStandardJob(job, BareMetalComputeService::JobTerminationCause::TERMINATED);

        // reply
        ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                job, this->getSharedPtr<BareMetalComputeService>(), true, nullptr,
                this->getMessagePayloadValue(
                        BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        return;
    }

    /**
     * @brief Helper function that determines whether there is at least one host with
     *        some number of cores (or more) and some ram capacity (or more)
     * @param num_cores: number of cores
     * @param ram: ram capacity
     * @return true is a host was found
     */
    bool BareMetalComputeService::isThereAtLeastOneHostWithResources(unsigned long num_cores, double ram) {

        for (auto const &r : this->compute_resources) {
            if ((std::get<0>(r.second) >= num_cores) and (std::get<1>(r.second) >= ram)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Helper method that determines wether a submitted job (with service-specific
     *        arguments) can run given available  resources
     *
     * @param job: the standard job
     * @param service_specific_arguments: the service-specific arguments
     * @return true if the job can run
     */
    bool BareMetalComputeService::jobCanRun(StandardJob *job,
                                            std::map<std::string, std::string> &service_specific_arguments) {

        for (auto t : job->getTasks()) {

            // No service-specific argument
            if ((service_specific_arguments.find(t->getID()) == service_specific_arguments.end()) or
                (service_specific_arguments[t->getID()].empty())) {
                if (not isThereAtLeastOneHostWithResources(t->getMinNumCores(), t->getMemoryRequirement())) {
                    return false;
                }
            }

            // Parse the service-specific argument
            std::tuple<std::string, unsigned long> parsed_spec = parseResourceSpec(
                    service_specific_arguments[t->getID()]);
            std::string desired_host = std::get<0>(parsed_spec);
            unsigned long desired_num_cores = std::get<1>(parsed_spec);

            if (desired_host.empty()) {
                // At this point the desired num cores in non-zero
                if (not isThereAtLeastOneHostWithResources(desired_num_cores, t->getMemoryRequirement())) {
                    return false;
                } else {
                    continue;
                }
            }

            // At this point the host is not empty
            unsigned long minimum_required_num_cores;
            if (desired_num_cores == 0) {
                minimum_required_num_cores = t->getMinNumCores();
            } else {
                minimum_required_num_cores = desired_num_cores;
            }
            unsigned long num_cores_on_desired_host = std::get<0>(this->compute_resources[desired_host]);
            double ram_on_desired_host = std::get<1>(this->compute_resources[desired_host]);
            if ((num_cores_on_desired_host < minimum_required_num_cores) or
                (ram_on_desired_host < t->getMemoryRequirement())) {
                return false;
            } else {
                continue;
            }
        }

        return true;
    }

/**
 * @brief Process a submit standard job request
 *
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 * @param job: the job
 * @param service_specific_args: service specific arguments
 *
 */
    void BareMetalComputeService::processSubmitStandardJob(
            const std::string &answer_mailbox, StandardJob *job,
            std::map<std::string, std::string> &service_specific_arguments) {
        WRENCH_INFO("Asked to run a standard job with %ld tasks", job->getNumTasks());

        // Do we support standard jobs?
        if (not this->supportsStandardJobs()) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitStandardJobAnswerMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(), false,
                            std::shared_ptr<FailureCause>(
                                    new JobTypeNotSupported(job, this->getSharedPtr<BareMetalComputeService>())),
                            this->getMessagePayloadValue(
                                    ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Can we run this job at all in terms of available resources?
        if (not jobCanRun(job, service_specific_arguments)) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitStandardJobAnswerMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(), false,
                            std::shared_ptr<FailureCause>(
                                    new NotEnoughResources(job, this->getSharedPtr<BareMetalComputeService>())),
                            this->getMessagePayloadValue(
                                    BareMetalComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD)));
            return;
        }

        // Construct the task run spec (i.e., keep track of service-specific arguments for each task)
        std::map<WorkflowTask *, std::tuple<std::string, unsigned long>> task_run_specs;
        for (auto t : job->getTasks()) {
            if ((service_specific_arguments.find(t->getID()) == service_specific_arguments.end()) or
                (service_specific_arguments[t->getID()].empty())) {
                task_run_specs.insert(std::make_pair(t, std::make_tuple("", 0)));
            } else {
                std::string spec = service_specific_arguments[t->getID()];
                task_run_specs.insert(std::make_pair(t, parseResourceSpec(spec)));
            }
        }

        // We can now admit the job!
        this->all_workunits.insert(std::make_pair(job, Workunit::createWorkunits(job)));
        this->job_run_specs.insert(std::make_pair(job, task_run_specs));

        // Add the ready ones to the ready list
        for (auto const &wu: this->all_workunits[job]) {
            if (wu->num_pending_parents == 0) {
                this->ready_workunits.push_back(wu);
            }
        }

        // Add the job to the list of running this
        this->running_jobs.insert(job);

        // And send a reply!
//        try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitStandardJobAnswerMessage(
                        job, this->getSharedPtr<BareMetalComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
//        } catch (std::shared_ptr<NetworkError> &cause) {
//            return;
//        }

    }

/**
 * @brief Process a submit pilot job request
 *
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void BareMetalComputeService::processSubmitPilotJob(const std::string &answer_mailbox,
                                                        PilotJob *job,
                                                        std::map<std::string, std::string> service_specific_args) {
        WRENCH_INFO("Asked to run a pilot job");

        if (not this->supportsPilotJobs()) {
//            try {
            S4U_Mailbox::dputMessage(
                    answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(), false,
                            std::shared_ptr<FailureCause>(
                                    new JobTypeNotSupported(job, this->getSharedPtr<BareMetalComputeService>())),
                            this->getMessagePayloadValue(
                                    BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
//            } catch (std::shared_ptr<NetworkError> &cause) {
//                return;
//            }
            return;
        }

        throw std::runtime_error(
                "BareMetalComputeService::processSubmitPilotJob(): We shouldn't be here! (fatal)");
    }

/**
 * @brief Process a "get resource description message"
 * @param answer_mailbox: the mailbox to which the description message should be sent
 */
    void BareMetalComputeService::processGetResourceInformation(const std::string &answer_mailbox) {
        // Build a dictionary
        std::map<std::string, std::map<std::string, double>> dict;

        // Num hosts
        std::map<std::string, double> num_hosts;
        num_hosts.insert(std::make_pair(this->getName(), this->compute_resources.size()));
        dict.insert(std::make_pair("num_hosts", num_hosts));

        // Num cores per hosts
        std::map<std::string, double> num_cores;
        for (auto r : this->compute_resources) {
            num_cores.insert(std::make_pair(r.first, (double) (std::get<0>(r.second))));
        }
        dict.insert(std::make_pair("num_cores", num_cores));

        // Num idle cores per hosts
        std::map<std::string, double> num_idle_cores;
        for (auto r : this->running_thread_counts) {
            unsigned long cores = std::get<0>(this->compute_resources[r.first]);
            unsigned long running_threads = r.second;
            num_idle_cores.insert(
                    std::make_pair(r.first, (double) (std::max<unsigned long>(cores - running_threads, 0))));
        }
        dict.insert(std::make_pair("num_idle_cores", num_idle_cores));

        // Flop rate per host
        std::map<std::string, double> flop_rates;
        for (auto h : this->compute_resources) {
            flop_rates.insert(std::make_pair(h.first, S4U_Simulation::getHostFlopRate(std::get<0>(h))));
        }
        dict.insert(std::make_pair("flop_rates", flop_rates));

        // RAM capacity per host
        std::map<std::string, double> ram_capacities;
        for (auto h : this->compute_resources) {
            ram_capacities.insert(std::make_pair(h.first, S4U_Simulation::getHostMemoryCapacity(std::get<0>(h))));
        }
        dict.insert(std::make_pair("ram_capacities", ram_capacities));

        // RAM availability per host
        std::map<std::string, double> ram_availabilities_to_return;
        for (auto r : this->ram_availabilities) {
            ram_availabilities_to_return.insert(std::make_pair(r.first, r.second));
        }
        dict.insert(std::make_pair("ram_availabilities", ram_availabilities_to_return));

        std::map<std::string, double> ttl;
        if (this->has_ttl) {
            ttl.insert(std::make_pair(this->getName(), this->death_date - S4U_Simulation::getClock()));
        } else {
            ttl.insert(std::make_pair(this->getName(), DBL_MAX));
        }
        dict.insert(std::make_pair("ttl", ttl));

        // Send the reply
        ComputeServiceResourceInformationAnswerMessage *answer_message = new ComputeServiceResourceInformationAnswerMessage(
                dict,
                this->getMessagePayloadValue(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
    }

/**
 * @brief Cleans up the scratch as I am a pilot job and I to need clean the files stored by the standard jobs
 *        executed inside me
 */
    void BareMetalComputeService::cleanUpScratch() {

        for (auto const &j : this->files_in_scratch) {
            for (auto const &f : j.second) {
                try {
                    StorageService::deleteFile(f,
                                               FileLocation::LOCATION(this->getScratch(),
                                                                      this->getScratch()->getMountPoint() +
                                                                      j.first->getName()));
                } catch (WorkflowExecutionException &e) {
                    throw;
                }
            }
        }
    }

/**
 * @brief Method to make sure that property specs are valid
 *
 * @throw std::invalid_argument
 */
    void BareMetalComputeService::validateProperties() {

        bool success = true;

        // Thread startup overhead
        double thread_startup_overhead = 0;
        try {
            thread_startup_overhead = this->getPropertyValueAsDouble(
                    BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD);
        } catch (std::invalid_argument &e) {
            success = false;
        }

        if ((!success) or (thread_startup_overhead < 0)) {
            throw std::invalid_argument("Invalid TASK_STARTUP_OVERHEAD property specification: " +
                                        this->getPropertyValueAsString(
                                                BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD));
        }

        // Supporting Pilot jobs
        if (this->getPropertyValueAsBoolean(BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS)) {
            throw std::invalid_argument(
                    "Invalid SUPPORTS_PILOT_JOBS property specification: a BareMetal Compute Service cannot support pilot jobs");
        }

    }

/**
 * @brief Not implement implemented. Will throw.
 * @param job: a pilot job to (supposedly) terminate
 *
 * @throw std::runtime_error
 */
    void BareMetalComputeService::terminatePilotJob(PilotJob *job) {
        throw std::runtime_error(
                "BareMetalComputeService::terminatePilotJob(): not implemented because BareMetalComputeService never supports pilot jobs");
    }


    /**
     * @brief Process a crash of a WorkunitExecutor (although some work may has been done, we'll just
     *        re-do the workunit from scratch)
     *
     * @param workunitExecutor: the workunit executor that has crashed
     */
    void BareMetalComputeService::processWorkunitExecutorCrash(std::shared_ptr<WorkunitExecutor> workunit_executor) {
        std::shared_ptr<Workunit> workunit = workunit_executor->workunit;

        WRENCH_INFO("Handling a WorkunitExecutor crash!");
        // Get the scratch files that executor may have generated
        StandardJob *job = workunit_executor->getJob();
        for (auto &f : workunit_executor->getFilesStoredInScratch()) {
            if (this->files_in_scratch.find(job) == this->files_in_scratch.end()) {
                this->files_in_scratch.insert(std::make_pair(job, (std::set<WorkflowFile *>) {}));
            }
            this->files_in_scratch[job].insert(f);
        }

        // Update RAM availabilities and running thread counts
        if (workunit->task) {
            this->ram_availabilities[workunit_executor->getHostname()] += workunit->task->getMemoryRequirement();
            this->running_thread_counts[workunit_executor->getHostname()] -= workunit_executor->getNumCores();
        }

        // Forget the workunit executor
        forgetWorkunitExecutor(workunit_executor);

        // Reset the internal task state to READY (it may have been completed actually, but we just redo the whole workunit)
        workunit->task->setInternalState(WorkflowTask::InternalState::TASK_READY);
        // Put the WorkUnit back in the ready list (at the end)
        WRENCH_INFO("Putting task back in the ready queue");
        this->ready_workunits.push_back(workunit);
    }

    /**
     * @brief A helper method to checks if all compute resources are down
     * @return true or false
     */
    bool BareMetalComputeService::areAllComputeResourcesDownWithNoWUERunning() {
        bool all_resources_down = true;
        for (auto const &h : this->compute_resources) {
            if (Simulation::isHostOn(h.first)) {
                all_resources_down = false;
                break;
            }
        }

        unsigned long num_running_wues = 0;
        for (auto const &job_wues : this->workunit_executors) {
            num_running_wues += job_wues.second.size();
        }

        return (all_resources_down and (num_running_wues == 0));
    }


};
