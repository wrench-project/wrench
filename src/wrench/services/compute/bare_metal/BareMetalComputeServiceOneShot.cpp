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

WRENCH_LOG_CATEGORY(wrench_core_bare_metal_compute_service, "Log category for bare_metal_standard_jobs");

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
    int BareMetalComputeService::main() {
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
//        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

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

        job->hasFailed();
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

        // Add the ttl
        dict["ttl"] = {{"ttl", this->ttl}};

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
            action->getJob()->hasFailed();
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
                this->current_jobs.erase(job);
                S4U_Mailbox::dputMessage(
                        job->popCallbackMailbox(),
                        new ComputeServiceCompoundJobDoneMessage(
                                job, this->getSharedPtr<BareMetalComputeService>(),
                                this->getMessagePayloadValue(
                                        BareMetalComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));

            } else if (job->hasFailed() and ((this->num_dispatched_actions_for_cjob[job] == 0))) {
                this->current_jobs.erase(job);
                S4U_Mailbox::putMessage(
                        job->popCallbackMailbox(),
                        new ComputeServiceCompoundJobFailedMessage(
                                job, this->getSharedPtr<BareMetalComputeService>(),
                                this->getMessagePayloadValue(
                                        BareMetalComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
            } else {
                // job is not one
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
