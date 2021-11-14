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
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
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

WRENCH_LOG_CATEGORY(wrench_core_bare_metal_compute_service, "Log category for bare_metal_standard_jobs");

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

        this->current_jobs.clear();
        this->not_ready_actions.clear();
        this->ready_actions.clear();
        this->dispatched_actions.clear();
    }

    /**
       * @brief Helper static method to parse resource specifications to the <cores,ram> format
       * @param spec: specification string
       * @return a <cores, ram> tuple
       * @throw std::invalid_argument
       */
    std::tuple<std::string, unsigned long> BareMetalComputeService::parseResourceSpec(const std::string &spec) {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, spec, boost::is_any_of(":"));
        switch (tokens.size()) {
            case 1: // "num_cores" or "hostname"
            {
                unsigned long num_threads;
                if (sscanf(tokens[0].c_str(), "%lu", &num_threads) != 1) {
                    return std::make_tuple(tokens[0], -1);
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
     * @brief Method the validates service-specific arguments (throws std::invalid_argument if invalid)
     * @param job: the job that's being submitted
     * @param service_specific_arg: the service-specific arguments
     */
    void BareMetalComputeService::validateServiceSpecificArguments(std::shared_ptr<Job> job,
                                                                const std::map<std::string, std::string> &service_specific_args) {

        auto cjob = std::dynamic_pointer_cast<CompoundJob>(job);
        auto compute_resources = this->action_execution_service->getComputeResources();
        // Check that each action can run w.r.t. the resource I have
        unsigned long max_cores = 0;
        double max_ram = 0;
        for (auto const &cr : compute_resources) {
            max_cores = (std::get<0>(cr.second) > max_cores ? std::get<0>(cr.second) : max_cores);
            max_ram = (std::get<1>(cr.second) > max_ram ? std::get<1>(cr.second) : max_ram);
        }

        // Validate that there are enough resources for each task
        for (auto const &action : cjob->getActions()) {
            if ((action->getMinRAMFootprint() > max_ram) or
                    (action->getMinNumCores() > max_cores)) {
                throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()));
            }
        }

        // Check that service-specific args make sense w.r.t to the resources I have
        for (auto const &action : cjob->getActions()) {
            if ((service_specific_args.find(action->getName()) != service_specific_args.end()) and
                (not service_specific_args.at(action->getName()).empty())) {
                std::tuple<std::string, unsigned long> parsed_spec;

                try {
                    parsed_spec = BareMetalComputeService::parseResourceSpec(service_specific_args.at(action->getName()));
                } catch (std::invalid_argument &e) {
                    throw;
                }

                std::string target_host = std::get<0>(parsed_spec);
                unsigned long target_num_cores = std::get<1>(parsed_spec);

//                std::cerr << "TARGET HOST " << target_host << "   TARGET CORES " << target_num_cores << "\n";

                if (not target_host.empty()) {

                    if (compute_resources.find(target_host) == compute_resources.end()) {
                        throw std::invalid_argument(
                                "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '" +
                                service_specific_args.at(action->getName()) +
                                "' for action '" + action->getName() + "': no such host");
                    }

                    if ((target_num_cores != -1) and (target_num_cores > std::get<0>(compute_resources[target_host]))) {
                        throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()));
                    }
                }

                if (target_num_cores != -1) {
                    if (target_num_cores < action->getMinNumCores()) {
                        throw std::invalid_argument(
                                "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '" +
                                service_specific_args.at(action->getName()) +
                                "' for action '" + action->getName() + "': the action requires more cores");
                    }

                    if (target_num_cores > action->getMaxNumCores()) {
                        throw std::invalid_argument(
                                "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '" +
                                service_specific_args.at(action->getName()) +
                                "' for action '" + action->getName() + "': the action cannot use this many cores");
                    }
                    if (target_num_cores > max_cores) {
                        throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()));
                    }
                }
            }
        }
    }

    /**
     * @brief Submit a compound job to the compute service
     * @param job: a compound job
     * @param service_specific_args: optional service specific arguments
     *
     *    These arguments are provided as a map of strings, indexed by action names. These
     *    strings are formatted as "[hostname:][num_cores]" (e.g., "somehost:12", "somehost","6", "").
     *
     *      - If a value is not provided for an action, then the service will choose a host and use as many cores as possible on that host.
     *      - If a "" value is provided for an action, then the service will choose a host and use as many cores as possible on that host.
     *      - If a "hostname" value is provided for an action, then the service will run the action on that
     *        host, using as many of its cores as possible
     *      - If a "num_cores" value is provided for an action, then the service will run that action with
     *        this many cores, but will choose the host on which to run it.
     *      - If a "hostname:num_cores" value is provided for an action, then the service will run that
     *        action with this many cores on that host.
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void BareMetalComputeService::submitCompoundJob(
            std::shared_ptr<CompoundJob> job,
            const std::map<std::string, std::string> &service_specific_args) {
        assertServiceIsUp();

        WRENCH_INFO("BareMetalComputeService::submitCompoundJob()");

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

        //  send a "run a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new ComputeServiceSubmitCompoundJobRequestMessage(
                                            answer_mailbox, job, service_specific_args,
                                            this->getMessagePayloadValue(
                                                    ComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceSubmitCompoundJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "ComputeService::submitCompoundJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }

    /**
     * @brief Asynchronously submit a pilot job to the compute service. This will raise
     *        a ExecutionException as this service does not support pilot jobs.
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments (only {} is supported)
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    void BareMetalComputeService::submitPilotJob(
            std::shared_ptr<PilotJob> job,
            const std::map<std::string, std::string> &service_specific_args) {
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
            throw ExecutionException(cause);
        }

        // Wait for a reply
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            } else {
                return;
            }

        } else {
            throw std::runtime_error(
                    "bare_metal_standard_jobs::submitPilotJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a map of <num_cores, memory_manager_service> tuples, indexed by hostname, which represents
     *        the compute resources available to this service.
     *          - use num_cores = ComputeService::ALL_CORES to use all cores available on the host
     *          - use memory_manager_service = ComputeService::ALL_RAM to use all RAM available on the host
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
    ) : ComputeService(hostname,
                       "bare_metal_standard_jobs",
                       "bare_metal_standard_jobs",
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
    ) : ComputeService(hostname,
                       "bare_metal_standard_jobs",
                       "bare_metal_standard_jobs",
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
    BareMetalComputeService::BareMetalComputeService(
            const std::string &hostname,
            std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            double ttl,
            std::shared_ptr<PilotJob> pj,
            std::string suffix, std::shared_ptr<StorageService> scratch_space
    ) : ComputeService(hostname,
                       "bare_metal_standard_jobs" + suffix,
                       "bare_metal_standard_jobs" + suffix,
                       scratch_space) {
        initiateInstance(hostname,
                         std::move(compute_resources),
                         std::move(property_list),
                         std::move(messagepayload_list),
                         ttl,
                         std::move(pj));
    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param compute_hosts:: a list of <hostname, num_cores, memory_manager_service> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param scratch_space: the scratch space for this compute service
     */
    BareMetalComputeService::BareMetalComputeService(
            const std::string &hostname,
            const std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list,
            std::shared_ptr<StorageService> scratch_space) :
            ComputeService(hostname,
                           "bare_metal_standard_jobs",
                           "bare_metal_standard_jobs",
                           scratch_space) {
        initiateInstance(hostname,
                         compute_resources,
                         std::move(property_list), std::move(messagepayload_list), DBL_MAX, nullptr);
    }

    /**
     * @brief Helper method called by all constructors to initiate object instance
     *
     * @param hostname: the name of the host
     * @param compute_resources: compute_resources: a map of <num_cores, memory_manager_service> pairs, indexed by hostname, which represent
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
            std::shared_ptr<PilotJob> pj) {
        if (ttl < 0) {
            throw std::invalid_argument(
                    "bare_metal_standard_jobs::initiateInstance(): invalid TTL value (must be >0)");
        }

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Validate that properties are correct
        this->validateProperties();

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        // Create an ActionExecutionService
        try {
            this->action_execution_service = std::shared_ptr<ActionExecutionService>(new ActionExecutionService(
                    hostname,
                    std::move(compute_resources),
                    {{ActionExecutionServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, this->getPropertyValueAsString(BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH)}},
                    {}
            ));
            this->action_execution_service->setSimulation(this->simulation);
        } catch (std::invalid_argument &e) {
            throw;
        }

        this->ttl = ttl;
        this->has_ttl = (this->ttl != DBL_MAX);
        this->containing_pilot_job = std::move(pj);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BareMetalComputeService::main() {
        this->state = Service::UP;


        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO("New BareMetal Compute Service starting");

        // Start the ActionExecutionService
        this->action_execution_service->setParentService(this->getSharedPtr<Service>());
        this->action_execution_service->setSimulation(this->simulation);
        this->action_execution_service->start(this->action_execution_service, true, false);

        // Set an alarm for my timely death, if necessary
        if (this->has_ttl) {
            this->death_date = S4U_Simulation::getClock() + this->ttl;
        }

        /** Main loop **/
        while (this->processNextMessage()) {
            dispatchReadyActions();
        }

        WRENCH_INFO("BareMetalService terminating cleanly!");
        return this->exit_code;
    }



    /**
     * @brief Wait for and react to any incoming message
     *x
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
        } catch (std::shared_ptr<NetworkError> &error) { WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->terminate();

            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                BareMetalComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitCompoundJobRequestMessage *>(message.get())) {
            processSubmitCompoundJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
            processGetResourceInformation(msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage *>(message.get())) {
            processIsThereAtLeastOneHostWithAvailableResources(msg->answer_mailbox, msg->num_cores, msg->ram);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceTerminateCompoundJobRequestMessage *>(message.get())) {
            processCompoundJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<ActionExecutionServiceActionDoneMessage *>(message.get())) {
            processActionDone(msg->action);
            return true;

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }


    /**
   * @brief Synchronously terminate a compound job previously submitted to the compute service
   *
   * @param job: a compound job
   *
   * @throw ExecutionException
   * @throw std::runtime_error
   */
    void BareMetalComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job) {
        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_compound_job");

        //  send a "terminate a compound job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new ComputeServiceTerminateCompoundJobRequestMessage(
                                            answer_mailbox, job, this->getMessagePayloadValue(
                                                    BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceTerminateCompoundJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "bare_metal_standard_jobs::terminateCompoundJob(): Received an unexpected [" +
                    message->getName() + "] message!");
        }
    }

    /**
    * @brief Process a submit compound job request
    *
    * @param answer_mailbox: the mailbox to which the answer message should be sent
    * @param job: the job
    * @param service_specific_args: service specific arguments
    *
    */
    void BareMetalComputeService::processSubmitCompoundJob(
            const std::string &answer_mailbox, std::shared_ptr<CompoundJob> job,
            std::map<std::string, std::string> &service_specific_arguments) {
        WRENCH_INFO("Asked to run a compound job with %ld actions", job->getActions().size());

        // Do we support standard jobs?
        if (not this->supportsCompoundJobs()) {
            auto failure_cause = std::shared_ptr<FailureCause>(
                    new JobTypeNotSupported(job, this->getSharedPtr<BareMetalComputeService>()));
            job->setAllActionsFailed(failure_cause);
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitCompoundJobAnswerMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(), false,
                            failure_cause,
                            this->getMessagePayloadValue(
                                    ComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }


        // Can we run this job at all in terms of available resources?
        bool can_run = true;
        for (auto const &action : job->getActions()) {
            if (not this->action_execution_service->actionCanRun(action)) {
                can_run = false;
                break;
            }
        }

        if (not can_run) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitCompoundJobAnswerMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(), false,
                            std::shared_ptr<FailureCause>(
                                    new NotEnoughResources(job, this->getSharedPtr<BareMetalComputeService>())),
                            this->getMessagePayloadValue(
                                    BareMetalComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD)));
            return;
        }

        // Add the job to the set of jobs
        this->num_dispatched_actions_for_cjob[job] = 0;
        this->current_jobs.insert(job);

        // Add all action to the list of actions to run
        for (auto const &action : job->getActions()) {
            if (action->getState() == Action::State::READY) {
                this->ready_actions.push_back(action);
            } else {
                this->not_ready_actions.insert(action);
            }
        }

        // And send a reply!
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitCompoundJobAnswerMessage(
                        job, this->getSharedPtr<BareMetalComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                ComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Terminate the daemon, dealing with pending/running job
     */
    void BareMetalComputeService::terminate() {
        this->setStateToDown();

        // Terminate all actions
        for (auto const &action : this->dispatched_actions) {
            this->action_execution_service->terminateAction(action);
        }

        // Deal with all jobs
        while (not this->current_jobs.empty()) {
            auto job = *(this->current_jobs.begin());
            try {
                this->current_jobs.erase(job);
                S4U_Mailbox::putMessage(
                        job->popCallbackMailbox(),
                        new ComputeServiceCompoundJobFailedMessage(
                                job, this->getSharedPtr<BareMetalComputeService>(),
                                this->getMessagePayloadValue(
                                        BareMetalComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return; // ignore
            }
        }

        cleanUpScratch();
    }


/**
 * @brief Process a compound job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void BareMetalComputeService::processCompoundJobTerminationRequest(std::shared_ptr<CompoundJob> job,
                                                                       const std::string &answer_mailbox) {

        // If the job doesn't exit, we reply right away
        if (this->current_jobs.find(job) == this->current_jobs.end()) {
            WRENCH_INFO(
                    "Trying to terminate a compound job that's not (no longer?) running!");
            std::string msg = "Job cannot be terminated because it is not running";
            auto answer_message = new ComputeServiceTerminateCompoundJobAnswerMessage(
                    job, this->getSharedPtr<BareMetalComputeService>(), false,
                    std::shared_ptr<FailureCause>(new NotAllowed(this->getSharedPtr<BareMetalComputeService>(), msg)),
                    this->getMessagePayloadValue(
                            BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
            return;
        }

        terminateRunningCompoundJob(job, BareMetalComputeService::JobTerminationCause::TERMINATED);

        // reply
        auto answer_message = new ComputeServiceTerminateCompoundJobAnswerMessage(
                job, this->getSharedPtr<BareMetalComputeService>(), true, nullptr,
                this->getMessagePayloadValue(
                        BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
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
                                                        std::shared_ptr<PilotJob> job,
                                                        std::map<std::string, std::string> service_specific_args) {
        WRENCH_INFO("Asked to run a pilot job");

        if (not this->supportsPilotJobs()) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(), false,
                            std::shared_ptr<FailureCause>(
                                    new JobTypeNotSupported(job, this->getSharedPtr<BareMetalComputeService>())),
                            this->getMessagePayloadValue(
                                    BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        throw std::runtime_error(
                "bare_metal_standard_jobs::processSubmitPilotJob(): We shouldn't be here! (fatal)");
    }

/**
 * @brief Process a host available resource request
 * @param answer_mailbox: the answer mailbox
 * @param num_cores: the desired number of cores
 * @param ram: the desired RAM
 */
    void BareMetalComputeService::processIsThereAtLeastOneHostWithAvailableResources(const std::string &answer_mailbox,
                                                                                     unsigned long num_cores,
                                                                                     double ram) {
        bool answer = this->action_execution_service->IsThereAtLeastOneHostWithAvailableResources(num_cores, ram);

        S4U_Mailbox::dputMessage(
                answer_mailbox, new ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage(
                        answer,
                        this->getMessagePayloadValue(
                                BareMetalComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD)));
    }

/**
 * @brief Process a "get resource description message"
 * @param answer_mailbox: the mailbox to which the description message should be sent
 */
    void BareMetalComputeService::processGetResourceInformation(const std::string &answer_mailbox) {
        auto dict = this->action_execution_service->getResourceInformation();

        // Send the reply
        auto *answer_message = new ComputeServiceResourceInformationAnswerMessage(
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
                    StorageService::deleteFile(f, FileLocation::LOCATION(
                            this->getScratch(),
                            this->getScratch()->getMountPoint() +
                            j.first->getName()));
                } catch (ExecutionException &e) {
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
                    "Invalid SUPPORTS_PILOT_JOBS property specification: a BareMetal Compute Service "
                    "cannot support pilot jobs");
        }
    }

/**
 * @brief Not implement implemented. Will throw.
 * @param job: a pilot job to (supposedly) terminate
 *
 * @throw std::runtime_error
 */
    void BareMetalComputeService::terminatePilotJob(std::shared_ptr<PilotJob> job) {
        throw std::runtime_error(
                "bare_metal_standard_jobs::terminatePilotJob(): not implemented because bare_metal_standard_jobs never supports pilot jobs");
    }


/**
 * @brief Helper method to dispatch actions
 */
    void BareMetalComputeService::dispatchReadyActions() {

//        std::cerr << "DISPACHING READY ACTIONS: |" << this->ready_actions.size() << " |\n";

        // Sort all the actions in the ready queue by (job.priority, action.priority)
        std::sort(this->ready_actions.begin(), this->ready_actions.end(),
                  [](const std::shared_ptr<Action> &a, const std::shared_ptr<Action> &b) -> bool {
                      if (a->getJob() != b->getJob()) {
                          if (a->getJob()->getPriority() > b->getJob()->getPriority()) {
                              return true;
                          } else if (a->getJob()->getPriority() < b->getJob()->getPriority()) {
                              return false;
                          } else {
                              return (unsigned long)(a->getJob().get()) > (unsigned long)(b->getJob().get());
                          }
                      } else {
                          if (a->getPriority() > b->getPriority()) {
                              return true;
                          } else if (a->getPriority() < b->getPriority()) {
                              return false;
                          } else {
                              return (unsigned long)(a.get()) > (unsigned long)(b.get());
                          }
                      }
                  });

        for (auto const &action : this->ready_actions) {
            this->action_execution_service->submitAction(action);
            this->num_dispatched_actions_for_cjob[action->getJob()]++;
            this->dispatched_actions.insert(action);
        }

        this->ready_actions.clear();
    }

/**
 * @brief Process an action completion
 * @param action
 */
    void BareMetalComputeService::processActionDone(std::shared_ptr<Action> action) {

//        for (auto const &a : this->dispatched_actions) {
//            WRENCH_INFO("DISPATCHED LIST: %s", a->getName().c_str());
//        }
        if (this->dispatched_actions.find(action) == this->dispatched_actions.end()) {
            WRENCH_INFO("Received a notification about action %s being done, but I don't know anything about this action - ignoring",
                        action->getName().c_str());
            return;
        }

//        std::cerr << "AN ACTION IS DONE: " << action->getName() << "\n";

        this->dispatched_actions.erase(action);
        this->num_dispatched_actions_for_cjob[action->getJob()]--;

        // Deal with action's ready children, if any
        for (auto const &child : action->getChildren()) {
            if (child->getState() == Action::State::READY) {
                this->not_ready_actions.erase(child);
                this->ready_actions.push_back(child);
            }
        }

        // Is the job done?
        auto job = action->getJob();
        try {
            if (job->hasSuccessfullyCompleted() and (this->num_dispatched_actions_for_cjob[job] == 0)) {
//                std::cerr << "JOB IS DONE!\n";
                this->current_jobs.erase(job);
                S4U_Mailbox::dputMessage(
                        job->popCallbackMailbox(),
                        new ComputeServiceCompoundJobDoneMessage(
                                job, this->getSharedPtr<BareMetalComputeService>(),
                                this->getMessagePayloadValue(
                                        BareMetalComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));

            } else if (job->hasFailed() and ((this->num_dispatched_actions_for_cjob[job] == 0))) {
//                std::cerr << "JOB HAS FAILED\n";
                this->current_jobs.erase(job);
                S4U_Mailbox::putMessage(
                        job->popCallbackMailbox(),
                        new ComputeServiceCompoundJobFailedMessage(
                                job, this->getSharedPtr<BareMetalComputeService>(),
                                this->getMessagePayloadValue(
                                        BareMetalComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
            } else {
//                std::cerr << "JOB IS NOT DONE\n";
            }
        } catch (std::shared_ptr<NetworkError> &cause) {
            return; // ignore
        }

    }

    void BareMetalComputeService::terminateRunningCompoundJob(std::shared_ptr<CompoundJob> job,
                                                              BareMetalComputeService::JobTerminationCause termination_cause) {
        for (auto const &action : job->getActions()) {
            if (this->dispatched_actions.find(action) != this->dispatched_actions.end()) {
                this->action_execution_service->terminateAction(action);
            }
        }
        this->current_jobs.erase(job);
    }

}
