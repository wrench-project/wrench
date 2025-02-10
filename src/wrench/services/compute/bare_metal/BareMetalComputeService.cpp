/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <map>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <memory>
#include <utility>

#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/job/PilotJob.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.h>

WRENCH_LOG_CATEGORY(wrench_core_bare_metal_compute_service, "Log category for bare_metal_compute_service");

namespace wrench
{
    /**
     * @brief Destructor
     */
    BareMetalComputeService::~BareMetalComputeService() = default;

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void BareMetalComputeService::cleanup(bool has_returned_from_main, int return_value)
    {
        // Do the default behavior (which will throw as this is not a fault-tolerant service)
        S4U_Daemon::cleanup(has_returned_from_main, return_value);

        this->action_execution_service = nullptr; // to avoid leak due to circular refs
        this->current_jobs.clear();
        this->not_ready_actions.clear();
        this->ready_actions.clear();
        this->dispatched_actions.clear();
    }

    /**
       * @brief Helper static method to parse resource specifications to the <cores,ram> format
       * @param spec: specification string
       * @return a <cores, ram> tuple
       */
    std::tuple<std::string, unsigned long> BareMetalComputeService::parseResourceSpec(const std::string& spec)
    {
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, spec, boost::is_any_of(":"));
        switch (tokens.size())
        {
        case 1: // "num_cores" or "hostname"
            {
                unsigned long num_threads;
                if (sscanf(tokens[0].c_str(), "%lu", &num_threads) != 1)
                {
                    return std::make_tuple(tokens[0], ULONG_MAX);
                }
                else
                {
                    return std::make_tuple(std::string(""), num_threads);
                }
            }
        case 2: // "hostname:num_cores"
            {
                unsigned long num_threads;
                if (sscanf(tokens[1].c_str(), "%lu", &num_threads) != 1)
                {
                    throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
                }
                return std::make_tuple(tokens[0], num_threads);
            }
        default:
            {
                throw std::invalid_argument("Invalid service-specific argument '" + spec + "'");
            }
        }
    }

    /**
     * @brief Method the validates service-specific arguments (throws std::invalid_argument if invalid)
     * @param job: the job that's being submitted
     * @param service_specific_args: the service-specific arguments
     */
    void BareMetalComputeService::validateServiceSpecificArguments(const std::shared_ptr<CompoundJob>& job,
                                                                   std::map<std::string, std::string>&
                                                                   service_specific_args)
    {
        auto cjob = std::dynamic_pointer_cast<CompoundJob>(job);
        auto compute_resources = this->action_execution_service->getComputeResources();
        // Check that each action can run w.r.t. the resource I have
        unsigned long max_cores = 0;
        sg_size_t max_ram = 0;
        for (auto const& cr : compute_resources)
        {
            max_cores = (std::get<0>(cr.second) > max_cores ? std::get<0>(cr.second) : max_cores);
            max_ram = (std::get<1>(cr.second) > max_ram ? std::get<1>(cr.second) : max_ram);
        }

        // Check that args are specified for existing tasks
        for (auto const& arg : service_specific_args)
        {
            if (not job->hasAction(arg.first))
            {
                throw std::invalid_argument(
                    "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '{"
                    +
                    arg.first + "," + arg.second + "}: no action named " + arg.first);
            }
        }

        // Validate that there are enough resources for each task
        for (auto const& action : cjob->getActions())
        {
            if ((action->getMinRAMFootprint() > max_ram) or
                (action->getMinNumCores() > max_cores))
            {
                throw ExecutionException(
                    std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()));
            }
        }

        // Check that service-specific args make sense w.r.t to the resources I have
        for (auto const& action : cjob->getActions())
        {
            if ((service_specific_args.find(action->getName()) != service_specific_args.end()) and
                (not service_specific_args.at(action->getName()).empty()))
            {
                std::tuple<std::string, unsigned long> parsed_spec;

                parsed_spec = BareMetalComputeService::parseResourceSpec(service_specific_args.at(action->getName()));

                auto target_host = S4U_Simulation::get_host_or_vm_by_name(std::get<0>(parsed_spec));
                unsigned long target_num_cores = std::get<1>(parsed_spec);

                if (target_host != nullptr)
                {
                    if (compute_resources.find(target_host) == compute_resources.end())
                    {
                        throw std::invalid_argument(
                            "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '"
                            +
                            service_specific_args.at(action->getName()) +
                            "' for action '" + action->getName() + "': no such host");
                    }

                    if ((target_num_cores != ULONG_MAX) and (target_num_cores > std::get<0>(
                        compute_resources[target_host])))
                    {
                        throw ExecutionException(
                            std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()));
                    }
                }

                if (target_num_cores != ULONG_MAX)
                {
                    if (target_num_cores < action->getMinNumCores())
                    {
                        throw std::invalid_argument(
                            "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '"
                            +
                            service_specific_args.at(action->getName()) +
                            "' for action '" + action->getName() + "': the action requires more cores");
                    }

                    if (target_num_cores > action->getMaxNumCores())
                    {
                        throw std::invalid_argument(
                            "BareMetalComputeService::validateServiceSpecificArguments(): Invalid service-specific argument '"
                            +
                            service_specific_args.at(action->getName()) +
                            "' for action '" + action->getName() + "': the action cannot use this many cores");
                    }
                    if (target_num_cores > max_cores)
                    {
                        throw ExecutionException(
                            std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()));
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
     *    strings are formatted as "[hostname:][num_cores]" (e.g., "some_host:12", "some_host","6", "").
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
     */
    void BareMetalComputeService::submitCompoundJob(
        std::shared_ptr<CompoundJob> job,
        const std::map<std::string, std::string>& service_specific_args)
    {
        assertServiceIsUp();

        //        WRENCH_INFO("BareMetalComputeService::submitCompoundJob()");

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
            new ComputeServiceSubmitCompoundJobRequestMessage(
                answer_commport, job, service_specific_args,
                this->getMessagePayloadValue(
                    ComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ComputeServiceSubmitCompoundJobAnswerMessage>(this->network_timeout,
            "ComputeService::submitCompoundJob(): Received an");
        if (not msg->success)
        {
            throw ExecutionException(msg->failure_cause);
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
        const std::string& hostname,
        const std::map<std::string, std::tuple<unsigned long, sg_size_t>>& compute_resources,
        const std::string& scratch_space_mount_point,
        const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
        const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) : ComputeService(hostname,
        "bare_metal",
        scratch_space_mount_point)
    {
        initiateInstance(hostname,
                         compute_resources,
                         property_list, messagepayload_list, nullptr);
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_hosts: the names of the hosts available as compute resources (the service
     *        will use all the cores and all the RAM of each host)
     * @param scratch_space_mount_point: the compute service's scratch space's mount point ("" means none)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    BareMetalComputeService::BareMetalComputeService(const std::string& hostname,
                                                     const std::vector<std::string>& compute_hosts,
                                                     const std::string& scratch_space_mount_point,
                                                     const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
                                                     const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) :
        ComputeService(hostname,
                       "bare_metal",
                       scratch_space_mount_point)
    {
        std::map<std::string, std::tuple<unsigned long, sg_size_t>> specified_compute_resources;
        for (const auto& h : compute_hosts)
        {
            specified_compute_resources[h] = std::make_tuple(ComputeService::ALL_CORES, ComputeService::ALL_RAM);
        }

        initiateInstance(hostname,
                         specified_compute_resources,
                         property_list, messagepayload_list, nullptr);
    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the service should be started
     * @param compute_resources: a list of <hostname, num_cores, memory_manager_service> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     * @param scratch_space: the scratch storage service
     *
     */
    BareMetalComputeService::BareMetalComputeService(
        const std::string& hostname,
        const std::map<std::string, std::tuple<unsigned long, sg_size_t>>& compute_resources,
        const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
        const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list,
        std::shared_ptr<PilotJob> pj,
        const std::string& suffix, std::shared_ptr<StorageService> scratch_space) : ComputeService(hostname,
        "bare_metal" + suffix,
        std::move(scratch_space))
    {
        initiateInstance(hostname,
                         compute_resources,
                         property_list,
                         messagepayload_list,
                         pj);
    }

    /**
     * @brief Internal constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param compute_resources: a list of <hostname, num_cores, memory_manager_service> tuples, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param scratch_space: the scratch space for this compute service
     */
    BareMetalComputeService::BareMetalComputeService(
        const std::string& hostname,
        const std::map<std::string, std::tuple<unsigned long, sg_size_t>>& compute_resources,
        const WRENCH_PROPERTY_COLLECTION_TYPE &property_list,
        const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE &messagepayload_list,
        std::shared_ptr<StorageService> scratch_space) : ComputeService(hostname,
                                                                        "bare_metal",
                                                                        std::move(scratch_space))
    {
        initiateInstance(hostname,
                         compute_resources,
                         property_list, messagepayload_list, nullptr);
    }

    /**
     * @brief Helper method called by all constructors to initiate object instance
     *
     * @param hostname: the name of the host
     * @param compute_resources: compute_resources: a map of <num_cores, memory_manager_service> pairs, indexed by hostname, which represent
     *        the compute resources available to this service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param pj: a containing PilotJob  (nullptr if none)
     *
     */
    void BareMetalComputeService::initiateInstance(
        const std::string& hostname,
        const std::map<std::string, std::tuple<unsigned long, sg_size_t>>& compute_resources,
        const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
        const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list,
        const std::shared_ptr<PilotJob>& pj)
    {
        // Set default and specified properties
        this->setProperties(this->default_property_values, property_list);

        // Validate that properties are correct
        this->validateProperties();

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        std::map<simgrid::s4u::Host*, std::tuple<unsigned long, sg_size_t>> specified_compute_resources;
        for (const auto& h : compute_resources)
        {
            specified_compute_resources[S4U_Simulation::get_host_or_vm_by_name(h.first)] = h.second;
        }

        // Create an ActionExecutionService
        this->action_execution_service = std::shared_ptr<ActionExecutionService>(new ActionExecutionService(
            hostname,
            specified_compute_resources,
            nullptr,
            {
                {
                    ActionExecutionServiceProperty::THREAD_CREATION_OVERHEAD,
                    this->getPropertyValueAsString(BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD)
                },
                {
                    ActionExecutionServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH,
                    this->getPropertyValueAsString(
                        BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH)
                },
                {
                    ActionExecutionServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN,
                    this->getPropertyValueAsString(
                        BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN)
                },
            },
            {}));
        this->action_execution_service->setSimulation(this->simulation_);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BareMetalComputeService::main()
    {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO("New BareMetal Compute Service starting");

        // Start the ActionExecutionService
        this->action_execution_service->setParentService(this->getSharedPtr<Service>());
        this->action_execution_service->setSimulation(this->simulation_);
        this->action_execution_service->start(this->action_execution_service, true, false);

        if (this->getPropertyValueAsBoolean(BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN))
        {
            // Set up a service termination detector for the action execution service if necessary
            auto termination_detector = std::make_shared<ServiceTerminationDetector>(
                this->hostname, this->action_execution_service,
                this->commport, false, true);
            termination_detector->setSimulation(this->simulation_);
            termination_detector->start(termination_detector, true, false); // Daemonized, no auto-restart
        }

        // Start the Scratch Storage Service
        this->startScratchStorageService();

        /** Main loop **/
        while (this->processNextMessage())
        {
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
     */
    bool BareMetalComputeService::processNextMessage()
    {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try
        {
            message = this->commport->getMessage();
        }
        catch (ExecutionException& e)
        {
            WRENCH_INFO(
                "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        //        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto ssd_msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message))
        {
            this->terminate(ssd_msg->send_failure_notifications,
                            (ComputeService::TerminationCause)(ssd_msg->termination_cause));

            // This is Synchronous
            try
            {
                ssd_msg->ack_commport->putMessage(
                    new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                        BareMetalComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            }
            catch (ExecutionException& e)
            {
                return false;
            }
            return false;
        }
        else if (auto csscjr_msg = std::dynamic_pointer_cast<ComputeServiceSubmitCompoundJobRequestMessage>(message))
        {
            processSubmitCompoundJob(csscjr_msg->answer_commport, csscjr_msg->job, csscjr_msg->service_specific_args);
            return true;
        }
        else if (auto csrir_msg = std::dynamic_pointer_cast<ComputeServiceResourceInformationRequestMessage>(message))
        {
            processGetResourceInformation(csrir_msg->answer_commport, csrir_msg->key);
            return true;
        }
        else if (auto csitalohwarr_msg = std::dynamic_pointer_cast<
            ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage>(message))
        {
            processIsThereAtLeastOneHostWithAvailableResources(csitalohwarr_msg->answer_commport,
                                                               csitalohwarr_msg->num_cores, csitalohwarr_msg->ram);
            return true;
        }
        else if (auto cstcjr_msg = std::dynamic_pointer_cast<ComputeServiceTerminateCompoundJobRequestMessage>(message))
        {
            processCompoundJobTerminationRequest(cstcjr_msg->job, cstcjr_msg->answer_commport);
            return true;
        }
        else if (auto aesad_msg = std::dynamic_pointer_cast<ActionExecutionServiceActionDoneMessage>(message))
        {
            processActionDone(aesad_msg->action);
            return true;
        }
        else if (auto sht_msg = std::dynamic_pointer_cast<ServiceHasTerminatedMessage>(message))
        {
            if (std::dynamic_pointer_cast<ActionExecutionService>(sht_msg->service))
            {
                if (this->getPropertyValueAsBoolean(
                    BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN))
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
            else
            {
                throw std::runtime_error(
                    "BareMetalComputeService::processNextMessage(): Received a service termination message for "
                    "a non-action-execution-service service");
            }
        }
        else
        {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }


    /**
   * @brief Synchronously terminate a compound job previously submitted to the compute service
   *
   * @param job: a compound job
   *
   */
    void BareMetalComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job)
    {
        assertServiceIsUp();

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        //  send a "terminate a compound job" message to the daemon's commport
        this->commport->putMessage(
            new ComputeServiceTerminateCompoundJobRequestMessage(
                answer_commport, job,
                this->getMessagePayloadValue(
                    BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ComputeServiceTerminateCompoundJobAnswerMessage>(
            "BareMetalComputeService::terminateCompoundJob(): Received an");
        if (not msg->success)
        {
            throw ExecutionException(msg->failure_cause);
        }
    }

    /**
    * @brief Process a submit compound job request
    *
    * @param answer_commport: the commport to which the answer message should be sent
    * @param job: the job
    * @param service_specific_arguments: service specific arguments
    *
    */
    void BareMetalComputeService::processSubmitCompoundJob(
        S4U_CommPort* answer_commport,
        const std::shared_ptr<CompoundJob>& job,
        std::map<std::string, std::string>& service_specific_arguments)
    {
        WRENCH_INFO("Asked to run compound job %s, which has %zu actions", job->getName().c_str(),
                    job->getActions().size());
        for (auto const& action : job->getActions())
        WRENCH_INFO("  - ACTION %s (min #cores=%lu)", action->getName().c_str(), action->getMinNumCores());

        // Action execution service may have terminated
        try
        {
            this->action_execution_service->assertServiceIsUp();
        }
        catch (ExecutionException& e)
        {
            // And send a reply!
            answer_commport->dputMessage(
                new ComputeServiceSubmitCompoundJobAnswerMessage(
                    job, this->getSharedPtr<BareMetalComputeService>(), false,
                    std::make_shared<ServiceIsDown>(this->getSharedPtr<BareMetalComputeService>()),
                    this->getMessagePayloadValue(
                        ComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Can we run this job at all in terms of available resources?
        bool can_run = true;
        for (auto const& action : job->getActions())
        {
            if (not this->action_execution_service->actionCanRun(action))
            {
                can_run = false;
                break;
            }
        }

        if (not can_run)
        {
            answer_commport->dputMessage(
                new ComputeServiceSubmitCompoundJobAnswerMessage(
                    job, this->getSharedPtr<BareMetalComputeService>(), false,
                    std::make_shared<NotEnoughResources>(job, this->getSharedPtr<BareMetalComputeService>()),
                    this->getMessagePayloadValue(
                        BareMetalComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD)));
            return;
        }

        // Add the job to the set of jobs
        this->num_dispatched_actions_for_cjob[job] = 0;
        this->current_jobs.insert(job);

        // Add all action to the list of actions to run
        for (auto const& action : job->getActions())
        {
            if (action->getState() == Action::State::READY)
            {
                this->ready_actions.push_back(action);
            }
            else
            {
                this->not_ready_actions.insert(action);
            }
        }


        // And send a reply!
        answer_commport->dputMessage(
            new ComputeServiceSubmitCompoundJobAnswerMessage(
                job, this->getSharedPtr<BareMetalComputeService>(), true, nullptr,
                this->getMessagePayloadValue(
                    ComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Terminate the daemon, dealing with pending/running job
     * @param send_failure_notifications: whether to send failure notifications
     * @param termination_cause: termination cause (if failure notifications are sent)
     */
    void BareMetalComputeService::terminate(bool send_failure_notifications,
                                            ComputeService::TerminationCause termination_cause)
    {
        this->setStateToDown();

        // Terminate all jobs
        for (auto const& job : this->current_jobs)
        {
            try
            {
                this->terminateCurrentCompoundJob(job, termination_cause);
            }
            catch (ExecutionException& e)
            {
                // If we get an exception, nevermind
            }
        }

        if (send_failure_notifications)
        {
            // Deal with all jobs
            while (not this->current_jobs.empty())
            {
                auto job = *(this->current_jobs.begin());
                try
                {
                    this->current_jobs.erase(job);
                    job->popCallbackCommPort()->dputMessage(
                        new ComputeServiceCompoundJobFailedMessage(
                            job, this->getSharedPtr<BareMetalComputeService>(),
                            this->getMessagePayloadValue(
                                BareMetalComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
                }
                catch (ExecutionException& e)
                {
                    return; // ignore
                }
            }
        }
        this->current_jobs.clear();

        cleanUpScratch();
    }


    /**
     * @brief Process a compound job termination request
     *
     * @param job: the job to terminate
     * @param answer_commport: the commport to which the answer message should be sent
     */
    void BareMetalComputeService::processCompoundJobTerminationRequest(const std::shared_ptr<CompoundJob>& job,
                                                                       S4U_CommPort* answer_commport)
    {
        // If the job doesn't exit, we reply right away
        if (this->current_jobs.find(job) == this->current_jobs.end())
        {
            WRENCH_INFO(
                "Trying to terminate a compound job that's not (no longer?) running!");
            std::string msg = "Job cannot be terminated because it is not running";
            auto answer_message = new ComputeServiceTerminateCompoundJobAnswerMessage(
                job, this->getSharedPtr<BareMetalComputeService>(), false,
                std::make_shared<NotAllowed>(this->getSharedPtr<BareMetalComputeService>(), msg),
                this->getMessagePayloadValue(
                    BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answer_message);
            return;
        }


        terminateCurrentCompoundJob(job, ComputeService::TerminationCause::TERMINATION_JOB_KILLED);
        this->current_jobs.erase(job);

        // reply
        auto answer_message = new ComputeServiceTerminateCompoundJobAnswerMessage(
            job, this->getSharedPtr<BareMetalComputeService>(), true, nullptr,
            this->getMessagePayloadValue(
                BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answer_message);
    }


    /**
 * @brief Process a host available resource request
 * @param answer_commport: the answer commport
 * @param num_cores: the desired number of cores
 * @param ram: the desired RAM
 */
    void BareMetalComputeService::processIsThereAtLeastOneHostWithAvailableResources(S4U_CommPort* answer_commport,
        unsigned long num_cores,
        sg_size_t ram)
    {
        bool answer = this->action_execution_service->IsThereAtLeastOneHostWithAvailableResources(num_cores, ram);
        answer_commport->dputMessage(
            new ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage(
                answer,
                this->getMessagePayloadValue(
                    BareMetalComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> BareMetalComputeService::constructResourceInformation(const std::string& key)
    {
        return this->action_execution_service->getResourceInformation(key);
    }

    /**
     * @brief Process a "get resource description message"
     * @param answer_commport: the commport to which the description message should be sent
     * @param key: the desired resource information (i.e., dictionary key) that's needed)
     */
    void BareMetalComputeService::processGetResourceInformation(S4U_CommPort* answer_commport,
                                                                const std::string& key)
    {
        std::map<std::string, double> dict;

        dict = this->constructResourceInformation(key);

        // Send the reply
        auto* answer_message = new ComputeServiceResourceInformationAnswerMessage(
            dict,
            this->getMessagePayloadValue(
                ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answer_message);
    }

    /**
 * @brief Cleans up the scratch as I am a pilot job and I to need clean the files stored by the standard jobs
 *        executed inside me
 */
    void BareMetalComputeService::cleanUpScratch()
    {
        for (auto const& j : this->files_in_scratch)
        {
            for (auto const& f : j.second)
            {
                this->getScratch()->deleteFile(
                    f,
                    this->getScratch()->getBaseRootPath() +
                    j.first->getName());
            }
        }
    }

    /**
 * @brief Method to make sure that property specs are valid
 *
 */
    void BareMetalComputeService::validateProperties()
    {
        bool success = true;

        // Thread startup overhead
        double thread_startup_overhead = 0;
        try
        {
            thread_startup_overhead = this->getPropertyValueAsTimeInSecond(
                BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD);
        }
        catch (std::invalid_argument& e)
        {
            success = false;
        }

        if ((!success) or (thread_startup_overhead < 0))
        {
            throw std::invalid_argument("Invalid THREAD_STARTUP_OVERHEAD property specification: " +
                this->getPropertyValueAsString(
                    BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD));
        }
    }


    /**
 * @brief Helper method to dispatch actions
 */
    void BareMetalComputeService::dispatchReadyActions()
    {
        //        std::cerr << "DISPATCHING READY ACTIONS: |" << this->ready_actions.size() << " |\n";

        // Sort all the actions in the ready queue by (job.priority, action.priority, action.job.submit_time, action.name)
        // TODO: This may be a performance bottleneck... may have to remedy
        std::sort(this->ready_actions.begin(), this->ready_actions.end(),
                  [](const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b) -> bool
                  {
                      if (a->getJob() != b->getJob())
                      {
                          if (a->getJob()->getPriority() > b->getJob()->getPriority()) {
                              return true;
                          } else if (a->getJob()->getPriority() < b->getJob()->getPriority()) {
                              return false;
                          } else if (a->getPriority() > b->getPriority()) {
                              return true;
                          } else if (a->getPriority() < b->getPriority()) {
                              return false;
                          } else if (a->getJob()->getSubmitDate() < b->getJob()->getSubmitDate()) {
                              return true;
                          } else if (a->getName() < b->getName()) {
                              return true;
                          } else if (a->getName() < b->getName()) {
                              return false;
                          } else {
                              return reinterpret_cast<unsigned long>(a->getJob().get()) > reinterpret_cast<unsigned
                                  long>(b->getJob().get());
                          }
                      } else {
                          if (a->getPriority() > b->getPriority()) {
                              return true;
                          } else if (a->getPriority() < b->getPriority()) {
                              return false;
                          } else if (a->getName() < b->getName()) {
                              return true;
                          } else if (a->getName() > b->getName()) {
                              return false;
                          } else {
                              return reinterpret_cast<unsigned long>(a.get()) > reinterpret_cast<unsigned long>(b.
                                  get());
                          }
                      }
                  });

        for (auto const& action : this->ready_actions)
        {
            if (this->dispatched_actions.find(action) != this->dispatched_actions.end()) {
                // The action has been dispatched (but its state it not set to RUNNING yet,
                // since there can be zero-size, instant communication!)
                break;
            }
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
    void BareMetalComputeService::processActionDone(const std::shared_ptr<Action>& action)
    {
        //        for (auto const &a : this->dispatched_actions) {
        //            WRENCH_INFO("DISPATCHED LIST: %s", a->getName().c_str());
        //        }
        if (this->dispatched_actions.find(action) == this->dispatched_actions.end()) {
            WRENCH_INFO(
                "Received a notification about action %s being done, but I don't know anything about this action - ignoring",
                action->getName().c_str());
            return;
        }

        WRENCH_INFO("Processing ActionDone: %s", action->getName().c_str());

        this->dispatched_actions.erase(action);
        this->num_dispatched_actions_for_cjob[action->getJob()]--;

        // Deal with action's ready children, if any
        for (auto const& child : action->getChildren()) {
            if (child->getState() == Action::State::READY) {
                this->not_ready_actions.erase(child);
                this->ready_actions.push_back(child);
            }
        }

        // Is the job done?
        auto job = action->getJob();

        try {
            if (job->hasSuccessfullyCompleted() and (this->num_dispatched_actions_for_cjob[job] == 0))
            {
                this->current_jobs.erase(job);
                this->num_dispatched_actions_for_cjob.erase(job);
                job->popCallbackCommPort()->dputMessage(
                    new ComputeServiceCompoundJobDoneMessage(
                        job, this->getSharedPtr<BareMetalComputeService>(),
                        this->getMessagePayloadValue(
                            BareMetalComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));
            }
            else if (job->hasFailed() and ((this->num_dispatched_actions_for_cjob[job] == 0)))
            {
                this->current_jobs.erase(job);
                this->num_dispatched_actions_for_cjob.erase(job);
                job->popCallbackCommPort()->dputMessage(
                    new ComputeServiceCompoundJobFailedMessage(
                        job, this->getSharedPtr<BareMetalComputeService>(),
                        this->getMessagePayloadValue(
                            BareMetalComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
            }
            else
            {
                // job is not one
            }
        }
        catch (ExecutionException& e)
        {
            return; // ignore
        }
    }

    /**
     * @brief Method for terminating a current compound job
     * @param job: the job
     * @param termination_cause: the termination cause
     */
    void BareMetalComputeService::terminateCurrentCompoundJob(const std::shared_ptr<CompoundJob>& job,
                                                              ComputeService::TerminationCause termination_cause)
    {
        for (auto const& action : job->getActions()) {
            if (this->dispatched_actions.find(action) != this->dispatched_actions.end())
            {
                this->action_execution_service->terminateAction(action, termination_cause);
            }
            else if (this->not_ready_actions.find(action) != this->not_ready_actions.end())
            {
                std::shared_ptr<FailureCause> failure_cause;
                switch (termination_cause)
                {
                case ComputeService::TerminationCause::TERMINATION_JOB_KILLED:
                    failure_cause = std::make_shared<JobKilled>(action->getJob());
                    break;
                case ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED:
                    failure_cause = std::make_shared<ServiceIsDown>(job->getParentComputeService());
                    break;
                case ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT:
                    failure_cause = std::make_shared<JobTimeout>(action->getJob());
                    break;
                default:
                    failure_cause = std::make_shared<JobKilled>(action->getJob());
                    break;
                }
                action->setFailureCause(failure_cause);
                this->not_ready_actions.erase(action);
            }
            else if (std::find(this->ready_actions.begin(), this->ready_actions.end(), action) != this->ready_actions.
                end())
            {
                std::shared_ptr<FailureCause> failure_cause;
                switch (termination_cause)
                {
                case ComputeService::TerminationCause::TERMINATION_JOB_KILLED:
                    failure_cause = std::make_shared<JobKilled>(action->getJob());
                    break;
                case ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED:
                    failure_cause = std::make_shared<ServiceIsDown>(job->getParentComputeService());
                    break;
                case ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT:
                    failure_cause = std::make_shared<JobTimeout>(action->getJob());
                    break;
                default:
                    failure_cause = std::make_shared<JobKilled>(action->getJob());
                    break;
                }
                action->setFailureCause(failure_cause);
                this->ready_actions.erase(std::find(this->ready_actions.begin(), this->ready_actions.end(), action));
            }
            else
            {
                // The action is already finished
            }
        }
        //        this->current_jobs.erase(job);
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool BareMetalComputeService::supportsStandardJobs()
    {
        return true;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool BareMetalComputeService::supportsCompoundJobs()
    {
        return true;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool BareMetalComputeService::supportsPilotJobs()
    {
        return false;
    }

    /**
     * @brief An "out of simulation time" (instant) method to check on resource availability
     * @param num_cores: desired number of cores
     * @param ram: desire RAM footprint
     * @return true if there is at least one host with the available free resources, false otherwise
     */
    bool BareMetalComputeService::isThereAtLeastOneHostWithIdleResourcesInstant(unsigned long num_cores, sg_size_t ram)
    {
        return this->action_execution_service->IsThereAtLeastOneHostWithAvailableResources(num_cores, ram);
    }
} // namespace wrench
