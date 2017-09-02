/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/MulticoreComputeService.h"

#include "wrench/simulation/Simulation.h"
#include "wrench/logging/TerminalOutput.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simulation/SimulationMessage.h"
#include "wrench/services/storage/StorageService.h"
#include "services/ServiceMessage.h"
#include "services/compute/ComputeServiceMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "workflow_job/PilotJob.h"
#include "MulticoreComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_compute_service, "Log category for Multicore Compute Service");

namespace wrench {

    /**
     * @brief Asynchronously submit a standard job to the compute service
     *
     * @param job: a standard job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     *
     */
    void MulticoreComputeService::submitStandardJob(StandardJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

      //  send a "run a standard job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceSubmitStandardJobRequestMessage(answer_mailbox, job,
                                                                                  this->getPropertyValueAsDouble(
                                                                                          MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (ComputeServiceSubmitStandardJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "MulticoreComputeService::submitStandardJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }

    }

    /**
     * @brief Asynchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void MulticoreComputeService::submitPilotJob(PilotJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

      // Send a "run a pilot job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceSubmitPilotJobRequestMessage(answer_mailbox, job,
                                                                               this->getPropertyValueAsDouble(
                                                                                       MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (ComputeServiceSubmitPilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "MulticoreComputeService::submitPilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }
    }


    /**
     * @brief Synchronously ask the service how many cores it has
     *
     * @return the number of cores
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long MulticoreComputeService::getNumCores() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "num cores" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_num_cores");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new MulticoreComputeServiceNumCoresRequestMessage(answer_mailbox,
                                                                                  this->getPropertyValueAsDouble(
                                                                                          MulticoreComputeServiceProperty::NUM_CORES_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (MulticoreComputeServiceNumCoresAnswerMessage *msg = dynamic_cast<MulticoreComputeServiceNumCoresAnswerMessage *>(message.get())) {
        return msg->num_cores;
      } else {
        throw std::runtime_error("MulticoreComputeService::getNumCores(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously ask the service how many idle cores it has
     *
     * @return the number of currently idle cores
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long MulticoreComputeService::getNumIdleCores() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "num idle cores" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_num_idle_cores");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new MulticoreComputeServiceNumIdleCoresRequestMessage(
                answer_mailbox,
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the reply
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (MulticoreComputeServiceNumIdleCoresAnswerMessage *msg = dynamic_cast<MulticoreComputeServiceNumIdleCoresAnswerMessage *>(message.get())) {
        return msg->num_idle_cores;
      } else {
        throw std::runtime_error(
                "MulticoreComputeService::getNumIdleCores(): unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously ask the service for its TTL
     *
     * @return the TTL in seconds
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    double MulticoreComputeService::getTTL() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "ttl request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_ttl");

      try {
        S4U_Mailbox::dputMessage(this->mailbox_name,
                                 new MulticoreComputeServiceTTLRequestMessage(
                                         answer_mailbox,
                                         this->getPropertyValueAsDouble(
                                                 MulticoreComputeServiceProperty::TTL_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (MulticoreComputeServiceTTLAnswerMessage *msg = dynamic_cast<MulticoreComputeServiceTTLAnswerMessage *>(message.get())) {
        return msg->ttl;
      } else {
        throw std::runtime_error("MulticoreComputeService::getTTL(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously ask the service for its per-core flop rate
     *
     * @return the rate in Flops/sec
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    double MulticoreComputeService::getCoreFlopRate() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "floprate request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_core_flop_rate");
      try {
        S4U_Mailbox::dputMessage(this->mailbox_name,
                                 new MulticoreComputeServiceFlopRateRequestMessage(
                                         answer_mailbox,
                                         this->getPropertyValueAsDouble(
                                                 MulticoreComputeServiceProperty::FLOP_RATE_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the reply
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (MulticoreComputeServiceFlopRateAnswerMessage *msg = dynamic_cast<MulticoreComputeServiceFlopRateAnswerMessage *>(message.get())) {
        return msg->flop_rate;
      } else {
        throw std::runtime_error(
                "MulticoreComputeService::getCoreFLopRate(): unexpected [" + msg->getName() + "] message");
      }
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param default_storage_service: a storage service (or nullptr)
     * @param plist: a property list
     */
    MulticoreComputeService::MulticoreComputeService(std::string hostname,
                                                     bool supports_standard_jobs,
                                                     bool supports_pilot_jobs,
                                                     StorageService *default_storage_service,
                                                     std::map<std::string, std::string> plist) :
            MulticoreComputeService::MulticoreComputeService(hostname,
                                                             supports_standard_jobs,
                                                             supports_pilot_jobs,
                                                             plist, 0, -1, nullptr, "",
                                                             default_storage_service) {

    }


    /**
     * @brief Constructor that starts the daemon for the service on a host,
     *        registering it with a WRENCH Simulation
     *
     * @param hostname: the name of the host
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param plist: a property list
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param ttl: the time-ti-live, in seconds
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     * @param default_storage_service: a storage service
     *
     * @throw std::invalid_argument
     */
    MulticoreComputeService::MulticoreComputeService(
            std::string hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            std::map<std::string, std::string> plist,
            unsigned long num_cores,
            double ttl,
            PilotJob *pj,
            std::string suffix,
            StorageService *default_storage_service) :
            ComputeService("multicore_compute_service" + suffix,
                           "multicore_compute_service" + suffix,
                           default_storage_service) {

      // Set default properties
      for (auto p : this->default_property_values) {
        this->setProperty(p.first, p.second);
      }

      // Set specified properties
      for (auto p : plist) {
        this->setProperty(p.first, p.second);
      }

      this->hostname = hostname;
      if (num_cores > 0) {
        this->num_available_cores = num_cores;
      } else {
        this->num_available_cores = S4U_Simulation::getNumCores(hostname);
      }
      this->ttl = ttl;
      this->has_ttl = (ttl >= 0);
      this->containing_pilot_job = pj;

      this->supports_standard_jobs = supports_standard_jobs;
      this->supports_pilot_jobs = supports_pilot_jobs;

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument &e) {
        throw;
      }
    }

/**
 * @brief Main method of the daemon
 *
 * @return 0 on termination
 */
    int MulticoreComputeService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_RED);

      WRENCH_INFO("New Multicore Job Executor starting (%s) with up to %ld worker threads ",
                  this->mailbox_name.c_str(), this->num_available_cores);

      this->death_date = -1.0;
      if (this->has_ttl) {
        this->death_date = S4U_Simulation::getClock() + this->ttl;
        WRENCH_INFO("Will be terminating at date %lf", this->death_date);
      }

      /** Main loop **/
      while (this->processNextMessage((this->has_ttl ? this->death_date - S4U_Simulation::getClock() : -1.0))) {

        /** Dispatch jobs **/
        while (this->dispatchNextPendingJob());
      }

      WRENCH_INFO("Multicore Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

/**
 * @brief Dispatch one pending job, if possible
 * @return true if a job was dispatched, false otherwise
 */
    bool MulticoreComputeService::dispatchNextPendingJob() {

      if (this->pending_jobs.size() == 0) {
        return false;
      }

      WorkflowJob *next_job = this->pending_jobs.front();

      /* Compute the min and max num cores for the job */
      unsigned long minimum_num_cores = 1;
      unsigned long maximum_num_cores = 0;

      switch (next_job->getType()) {
        case WorkflowJob::STANDARD: {
          for (auto t : ((StandardJob *)next_job)->getTasks()) {
            minimum_num_cores = MAX(minimum_num_cores, t->getMinNumCores());
            maximum_num_cores += t->getMaxNumCores();
          }
          maximum_num_cores = MAX(maximum_num_cores, 1);
          break;
        }
        case WorkflowJob::PILOT: {
          minimum_num_cores = ((PilotJob *)next_job)->getNumCores();
          maximum_num_cores = ((PilotJob *)next_job)->getNumCores();
          break;
        }
      }

      /* See whether the job can run */
      if (this->num_available_cores < minimum_num_cores) {
        return false;
      }

      /* Allocate resources to the job */
      unsigned long num_allocated_cores;
      if (this->getPropertyValueAsString(MulticoreComputeServiceProperty::CORE_ALLOCATION_POLICY) == "aggressive") {
        num_allocated_cores = MIN(num_available_cores, maximum_num_cores);
      }

      // Remove the job from the pending job list
      this->pending_jobs.pop_back();

      switch (next_job->getType()) {

        case WorkflowJob::STANDARD: {

          WRENCH_INFO("Creating a StandardJobExecutor on %ld cores for a standard job", num_allocated_cores);
          // Create a standard job executor
          StandardJobExecutor *executor = new StandardJobExecutor(
                  this->simulation,
                  this->mailbox_name,
                  this->hostname,
                  (StandardJob *)next_job,
                  {std::pair<std::string, unsigned long>{this->hostname, num_allocated_cores}},
                  this->default_storage_service,
                  {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
                           this->getPropertyValueAsString(MulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD)}});

          this->standard_job_executors.insert(executor);
          this->running_jobs.insert(next_job);

          return true;
        }

        case WorkflowJob::PILOT: {
          PilotJob *job = (PilotJob *) next_job;
          WRENCH_INFO("Allocating %ld cores to a pilot job", num_allocated_cores);

          // Immediately decrease the number of available worker threads
          this->num_available_cores -= num_allocated_cores;

          ComputeService *cs =
                  new MulticoreComputeService(S4U_Simulation::getHostName(),
                                              true, false, this->property_list,
                                              (unsigned int) job->getNumCores(),
                                              job->getDuration(), job,
                                              "_pilot",
                                              this->default_storage_service);
          cs->setSimulation(this->simulation);

          // Create and launch a compute service for the pilot job
          job->setComputeService(cs);

          // Put the job in the running queue
          this->running_jobs.insert(next_job);

          // Send the "Pilot job has started" callback
          // Note the getCallbackMailbox instead of the popCallbackMailbox, because
          // there will be another callback upon termination.
          try {
            S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
                                     new ComputeServicePilotJobStartedMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      MulticoreComputeServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
          }

          // Push my own mailbox onto the pilot job!
          job->pushCallbackMailbox(this->mailbox_name);
          return true;
        }
          break;
      }
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
    bool MulticoreComputeService::processNextMessage(double timeout) {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message;

      try {
        if (this->has_ttl) {
          if (timeout <= 0) {
            return false;
          } else {
            message = S4U_Mailbox::getMessage(this->mailbox_name, timeout);
          }
        } else {
          message = S4U_Mailbox::getMessage(this->mailbox_name);
        }
      } catch (std::shared_ptr<NetworkTimeout> cause) {
        WRENCH_INFO("Time out - must die.. !!");
        this->terminate(true);
        return false;
      } catch (std::shared_ptr<NetworkError> cause) {
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->terminate(false);
        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          MulticoreComputeServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
          return false;
        }
        return false;

      } else if (ComputeServiceSubmitStandardJobRequestMessage *msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        WRENCH_INFO("Asked to run a standard job with %ld tasks", msg->job->getNumTasks());
        if (not this->supportsStandardJobs()) {
          try {
            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new ComputeServiceSubmitStandardJobAnswerMessage(msg->job, this,
                                                                                      false,
                                                                                      std::shared_ptr<FailureCause>(new JobTypeNotSupported(msg->job,
                                                                                                                                            this)),
                                                                                      this->getPropertyValueAsDouble(
                                                                                              MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            return true;
          }
          return true;
        }

//        for (auto task : msg->job->getTasks()) {
//          if ((task->getMinNumCores() > 1)) {
//            throw std::runtime_error("MulticoreComputeService currently does not support multi-core tasks, and task " +
//                                     task->getId() + " needs at least " +
//                                     std::to_string(task->getMinNumCores()) +
//                                     " cores");
//          }
//        }

        this->pending_jobs.push_front(msg->job);

        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                   new ComputeServiceSubmitStandardJobAnswerMessage(msg->job, this,
                                                                                    true,
                                                                                    nullptr,
                                                                                    this->getPropertyValueAsDouble(
                                                                                            MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
          return true;
        }
        return true;

      } else if (ComputeServiceSubmitPilotJobRequestMessage *msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        WRENCH_INFO("Asked to run a pilot job with %ld cores for %lf seconds", msg->job->getNumCores(),
                    msg->job->getDuration());

        bool success = true;

        if (not this->supportsPilotJobs()) {
          try {
            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new ComputeServiceSubmitPilotJobAnswerMessage(msg->job,
                                                                                   this,
                                                                                   false,
                                                                                   std::shared_ptr<FailureCause>(new JobTypeNotSupported(msg->job,
                                                                                                                                         this)),
                                                                                   this->getPropertyValueAsDouble(
                                                                                           MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            return true;
          }
          return true;
        }

        if (S4U_Simulation::getNumCores(this->hostname) < msg->job->getNumCores()) {

          try {
            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new ComputeServiceSubmitPilotJobAnswerMessage(msg->job,
                                                                                   this,
                                                                                   false,
                                                                                   std::shared_ptr<FailureCause>(new NotEnoughCores(msg->job, this)),
                                                                                   this->getPropertyValueAsDouble(
                                                                                           MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          } catch (std::shared_ptr<NetworkError> cause) {
            return true;
          }
          return true;
        }

        // success
        this->pending_jobs.push_front(msg->job);
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                   new ComputeServiceSubmitPilotJobAnswerMessage(msg->job,
                                                                                 this,
                                                                                 true,
                                                                                 nullptr,
                                                                                 this->getPropertyValueAsDouble(
                                                                                         MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
          return true;
        }
        return true;

      } else if (ComputeServicePilotJobExpiredMessage *msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        processPilotJobCompletion(msg->job);
        return true;

      } else if (MulticoreComputeServiceNumCoresRequestMessage *msg = dynamic_cast<MulticoreComputeServiceNumCoresRequestMessage *>(message.get())) {
        MulticoreComputeServiceNumCoresAnswerMessage *answer_message = new MulticoreComputeServiceNumCoresAnswerMessage(
                S4U_Simulation::getNumCores(this->hostname),
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::NUM_CORES_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> cause) {
          return true;
        }
        return true;
      } else if (MulticoreComputeServiceNumIdleCoresRequestMessage *msg = dynamic_cast<MulticoreComputeServiceNumIdleCoresRequestMessage *>(message.get())) {
        MulticoreComputeServiceNumIdleCoresAnswerMessage *answer_message = new MulticoreComputeServiceNumIdleCoresAnswerMessage(
                this->num_available_cores,
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> cause) {
          return true;
        }
        return true;
      } else if (MulticoreComputeServiceTTLRequestMessage *msg = dynamic_cast<MulticoreComputeServiceTTLRequestMessage *>(message.get())) {
        MulticoreComputeServiceTTLAnswerMessage *answer_message = new MulticoreComputeServiceTTLAnswerMessage(
                this->death_date - S4U_Simulation::getClock(),
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::TTL_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> cause) {
          return true;
        }
        return true;
      } else if (MulticoreComputeServiceFlopRateRequestMessage *msg = dynamic_cast<MulticoreComputeServiceFlopRateRequestMessage *>(message.get())) {
        MulticoreComputeServiceFlopRateAnswerMessage *answer_message = new MulticoreComputeServiceFlopRateAnswerMessage(
                S4U_Simulation::getFlopRate(this->hostname),
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::FLOP_RATE_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(msg->answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> cause) {
          return true;
        }
        return true;

      } else if (ComputeServiceTerminateStandardJobRequestMessage *msg = dynamic_cast<ComputeServiceTerminateStandardJobRequestMessage *>(message.get())) {

        processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (ComputeServiceTerminatePilotJobRequestMessage *msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {

        processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
        return true;

      } else if (StandardJobExecutorDoneMessage *msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
        processStandardJobCompletion(msg->executor, msg->job);
        return true;

      } else if (StandardJobExecutorFailedMessage *msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
        processStandardJobFailure(msg->executor, msg->job, msg->cause);
        return true;

      } else {
        throw std::runtime_error("Unexpected [" + message->getName() + "] message");
      }
    }

/**
 * @brief Terminate all pilot job compute services
 */
    void MulticoreComputeService::terminateAllPilotJobs() {
      for (auto job : this->running_jobs) {
        if (job->getType() == WorkflowJob::PILOT) {
          PilotJob *pj = (PilotJob *) job;
          WRENCH_INFO("Terminating pilot job %s", job->getName().c_str());
          pj->getComputeService()->stop();
        }
      }
    }

/**
 * @brief fail a pending standard job
 * @param job: the job
 * @param cause: the failure cause
 */
    void MulticoreComputeService::failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {
      WRENCH_INFO("Failing pending job %s", job->getName().c_str());
      // Set all tasks back to the READY state and wipe out output files
      for (auto failed_task: job->getTasks()) {
        failed_task->setReady();
        try {
          StorageService::deleteFiles(failed_task->getOutputFiles(), job->getFileLocations(),
                                      this->default_storage_service);
        } catch (WorkflowExecutionException &e) {
          WRENCH_WARN("Warning: %s", e.getCause()->toString().c_str());
        }
      }

      // Send back a job failed message
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(job, this, cause,
                                                                           this->getPropertyValueAsDouble(
                                                                                   MulticoreComputeServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }
    }

/**
 * @brief fail a running standard job
 * @param job: the job
 * @param cause: the failure cause
 */
    void MulticoreComputeService::failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {

      WRENCH_INFO("Failing running job %s", job->getName().c_str());

      terminateRunningStandardJob(job);

      // Send back a job failed message (Not that it can be a partial fail)
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      try {
        S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                new ComputeServiceStandardJobFailedMessage(job, this, cause,
                                                                           this->getPropertyValueAsDouble(
                                                                                   MulticoreComputeServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }
    }

/**
* @brief terminate a running standard job
* @param job: the job
*/
    void MulticoreComputeService::terminateRunningStandardJob(StandardJob *job) {

      StandardJobExecutor *executor = nullptr;
      for (auto e : this->standard_job_executors) {
        if (e->getJob() == job) {
          executor = e;
        }
      }
      if (executor == nullptr) {
        throw std::runtime_error("MulticoreComputeService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
      }

      // Terminate the executor
      WRENCH_INFO("Terminating a standard job executor");
      executor->kill();

      // Set all non-COMPLETED tasks back to the READY state and wipe out output files
      // TODO: At some point we'll have to think hard about the task life cycle and make it better/coherent
      for (auto failed_task: job->getTasks()) {
//        WRENCH_INFO("====> %s %s", failed_task->getId().c_str(), WorkflowTask::stateToString(failed_task->getState()).c_str());
        switch (failed_task->getState()) {
          case WorkflowTask::NOT_READY: {
            break;
          }
          case WorkflowTask::READY: {
            break;
          }
          case WorkflowTask::PENDING: {
            failed_task->setReady();
            break;
          }
          case WorkflowTask::RUNNING: {
            failed_task->setFailed();
            failed_task->setReady();
            break;
          }
          case WorkflowTask::COMPLETED: {
            break;
          }
          case WorkflowTask::FAILED: {
            failed_task->setReady();
            break;
          }
          default: {
            throw std::runtime_error("MulticoreComputeService::terminateRunningStandardJob(): unexpected task state");
          }
        }
      }

      return;
    }

/**
 * @brief Terminate a running pilot job
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void MulticoreComputeService::terminateRunningPilotJob(PilotJob *job) {

      // Get the associated compute service
      ComputeService *compute_service = job->getComputeService();

      if (compute_service == nullptr) {
        throw std::runtime_error("MulticoreComputeService::terminateRunningPilotJob(): can't find compute service associated to pilot job");
      }

      // Stop it
      compute_service->stop();

      // Remove the job from the running list
      this->running_jobs.erase(job);

      // Update the number of available cores
      this->num_available_cores += job->getNumCores();
    }


/**
* @brief Declare all current jobs as failed (likely because the daemon is being terminated
* or has timed out (because it's in fact a pilot job))
*/
    void MulticoreComputeService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {

      for (auto workflow_job : this->running_jobs) {
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          this->failRunningStandardJob(job, std::move(cause));
        }
      }

      while (not this->pending_jobs.empty()) {
        WorkflowJob *workflow_job = this->pending_jobs.front();
        this->pending_jobs.pop_back();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          this->failPendingStandardJob(job, cause);
        }
      }
    }


/**
 * @brief Process a standard job completion
 * @param executor: the standard job executor
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void MulticoreComputeService::processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job) {


      // Remove the executor from the executor list
      WRENCH_INFO("====> %ld", this->standard_job_executors.size());
      if (this->standard_job_executors.find(executor) == this->standard_job_executors.end()) {
        throw std::runtime_error("MulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }
      this->standard_job_executors.erase(executor);

      // Remove the job from the running job list
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error("MulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }
      this->running_jobs.erase(job);

      WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

      // Send the callback to the originator
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServiceStandardJobDoneMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  MulticoreComputeServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }

      return;
    }


/**
 * @brief Process a work failure
 * @param worker_thread: the worker thread that did the work
 * @param work: the work
 * @param cause: the cause of the failure
 */
    void MulticoreComputeService::processStandardJobFailure(StandardJobExecutor *executor,
                                                            StandardJob *job,
                                                            std::shared_ptr<FailureCause> cause) {

      // Remove the executor from the executor list
      if (this->standard_job_executors.find(executor) == this->standard_job_executors.end()) {
        throw std::runtime_error("MulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
      }
      this->standard_job_executors.erase(executor);

      // Remove the job from the running job list
      if (this->running_jobs.find(job) == this->running_jobs.end()) {
        throw std::runtime_error("MulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
      }
      this->running_jobs.erase(job);

      WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

      // Fail the job
      this->failPendingStandardJob(job, cause);

    }

/**
 * @brief Terminate the daemon, dealing with pending/running jobs
 */
    void MulticoreComputeService::terminate(bool notify_pilot_job_submitters) {

      this->setStateToDown();

      WRENCH_INFO("Failing current standard jobs");
      this->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));

      WRENCH_INFO("Terminate all pilot jobs");
      this->terminateAllPilotJobs();

      // Am I myself a pilot job?
      if (notify_pilot_job_submitters && this->containing_pilot_job) {

        WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox %s",
                    this->containing_pilot_job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
          S4U_Mailbox::putMessage(this->containing_pilot_job->popCallbackMailbox(),
                                  new ComputeServicePilotJobExpiredMessage(this->containing_pilot_job, this,
                                                                           this->getPropertyValueAsDouble(
                                                                                   MulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
          return;
        }
      }
    }

/**
 * @brief Process a pilot job completion
 *
 * @param job: the pilot job
 */
    void MulticoreComputeService::processPilotJobCompletion(PilotJob *job) {

      // Remove the job from the running list
      this->running_jobs.erase(job);

      // Update the number of available cores
      this->num_available_cores += job->getNumCores();

      // Forward the notification
      try {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, this,
                                                                          this->getPropertyValueAsDouble(
                                                                                  MulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }

      return;
    }


/**
 * @brief Synchronously terminate a standard job previously submitted to the compute service
 *
 * @param job: the standard job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 *
 */
    void MulticoreComputeService::terminateStandardJob(StandardJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

      //  send a "terminate a standard job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminateStandardJobRequestMessage(answer_mailbox, job,
                                                                                     this->getPropertyValueAsDouble(
                                                                                             MulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (ComputeServiceTerminateStandardJobAnswerMessage *msg = dynamic_cast<ComputeServiceTerminateStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "MulticoreComputeService::terminateStandardJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }

    }

/**
 * @brief Synchronously terminate a pilot job to the compute service
 *
 * @param job: a pilot job
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 */
    void MulticoreComputeService::terminatePilotJob(PilotJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_pilot_job");

      // Send a "terminate a pilot job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox, job,
                                                                                  this->getPropertyValueAsDouble(
                                                                                          MulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }

      if (ComputeServiceTerminatePilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceTerminatePilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "MulticoreComputeService::terminatePilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }
    }


/**
 * @brief Process a standard job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void MulticoreComputeService::processStandardJobTerminationRequest(StandardJob *job, std::string answer_mailbox) {

      // Check whether job is pending
      for (auto it = this->pending_jobs.begin(); it < this->pending_jobs.end(); it++) {
        if (*it == job) {
          this->pending_jobs.erase(it);
          ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          MulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> cause) {
            return;
          }
          return;
        }
      }

      // Check whether the job is running
      if (this->running_jobs.find(job) != this->running_jobs.end()) {
        this->running_jobs.erase(job);
        terminateRunningStandardJob(job);
        ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
                job, this, true, nullptr,
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> cause) {
          return;
        }
        return;
      }

      // If we got here, we're in trouble
      WRENCH_INFO("Trying to terminate a standard job that's neither pending nor running!");
      ComputeServiceTerminateStandardJobAnswerMessage *answer_message = new ComputeServiceTerminateStandardJobAnswerMessage(
              job, this, false,  std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
              this->getPropertyValueAsDouble(
                      MulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }
      return;
    }

/**
 * @brief Process a pilot job termination request
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void MulticoreComputeService::processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox) {


      // Check whether job is pending
      for (auto it = this->pending_jobs.begin(); it < this->pending_jobs.end(); it++) {
        if (*it == job) {
          this->pending_jobs.erase(it);
          ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                  job, this, true, nullptr,
                  this->getPropertyValueAsDouble(
                          MulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
          try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
          } catch (std::shared_ptr<NetworkError> cause) {
            return;
          }
          return;
        }
      }

      // Check whether the job is running
      if (this->running_jobs.find(job) != this->running_jobs.end()) {
        this->running_jobs.erase(job);
        terminateRunningPilotJob(job);
        ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                job, this, true, nullptr,
                this->getPropertyValueAsDouble(
                        MulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
          S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> cause) {
          return;
        }
        return;
      }

      // If we got here, we're in trouble
      WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
      ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
              job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
              this->getPropertyValueAsDouble(
                      MulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }
      return;
    }

};

