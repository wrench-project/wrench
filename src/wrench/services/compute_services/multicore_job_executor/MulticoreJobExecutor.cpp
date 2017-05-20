/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "MulticoreJobExecutor.h"

#include <simulation/Simulation.h>
#include <workflow_job/StandardJob.h>
#include <logging/TerminalOutput.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simulation/SimulationMessage.h>
#include <services/storage_services/StorageService.h>
#include <helper_daemons/sequential_task_executor/SequentialTaskExecutorMessage.h>
#include <services/ServiceMessage.h>
#include <services/compute_services/ComputeServiceMessage.h>
#include "exceptions/WorkflowExecutionException.h"
#include "workflow_job/PilotJob.h"
#include "MulticoreJobExecutorMessage.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_job_executor, "Log category for Multicore Job Executor");


namespace wrench {

    /**
     * @brief Asynchronously submit a standard job to the compute service
     *
     * @param job: a pointer a StandardJob object
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     *
     */
    void MulticoreJobExecutor::submitStandardJob(StandardJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();

      //  send a "run a task" message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name,
                       new ComputeServiceSubmitStandardJobRequestMessage(answer_mailbox, job, this->getPropertyValueAsDouble(
                               MulticoreJobExecutorProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));

      // Get the answer
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (ComputeServiceSubmitStandardJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "MulticoreJobExecutor::submitStandardJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }

    };

    /**
     * @brief Asynchronously submit a pilot job to the compute service
     *
     * @param task: a pointer the PilotJob object
     * @param callback_mailbox: the name of a mailbox to which a "pilot job started" callback will be sent
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void MulticoreJobExecutor::submitPilotJob(PilotJob *job) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();

      //  send a "run a task" message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name,
                       new ComputeServiceSubmitPilotJobRequestMessage(answer_mailbox, job, this->getPropertyValueAsDouble(
                               MulticoreJobExecutorProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));

      // Get the answer
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (ComputeServiceSubmitPilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (!msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }

      } else {
        throw std::runtime_error(
                "MulticoreJobExecutor::submitPilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }

    };

    /**
     * @brief Synchronously ask the service how many cores it has
     *
     * @return the number of cores
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long MulticoreJobExecutor::getNumCores() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "num cores" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::put(this->mailbox_name,
                       new MulticoreJobExecutorNumCoresRequestMessage(answer_mailbox,
                                                  this->getPropertyValueAsDouble(
                                                          MulticoreJobExecutorProperty::NUM_CORES_REQUEST_MESSAGE_PAYLOAD)));

      // get the reply
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (MulticoreJobExecutorNumCoresAnswerMessage *msg = dynamic_cast<MulticoreJobExecutorNumCoresAnswerMessage *>(message.get())) {
        return msg->num_cores;
      } else {
        throw std::runtime_error(
                "MulticoreJobExecutor::getNumCores(): unexpected [" + msg->getName() + "] message");
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
    unsigned long MulticoreJobExecutor::getNumIdleCores() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "num idle cores" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();

      S4U_Mailbox::dput(this->mailbox_name, new MulticoreJobExecutorNumIdleCoresRequestMessage(
              answer_mailbox,
              this->getPropertyValueAsDouble(MulticoreJobExecutorProperty::NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD)));

      // Get the reply
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (MulticoreJobExecutorNumIdleCoresAnswerMessage *msg = dynamic_cast<MulticoreJobExecutorNumIdleCoresAnswerMessage *>(message.get())) {
        return msg->num_idle_cores;
      } else {
        throw std::runtime_error(
                "MulticoreJobExecutor::getNumIdleCores(): unexpected [" + msg->getName() + "] message");
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
    double MulticoreJobExecutor::getTTL() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "ttl request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::dput(this->mailbox_name,
                        new MulticoreJobExecutorTTLRequestMessage(
                                answer_mailbox,
                                this->getPropertyValueAsDouble(
                                        MulticoreJobExecutorProperty::TTL_REQUEST_MESSAGE_PAYLOAD)));

      // Get the reply
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (MulticoreJobExecutorTTLAnswerMessage *msg = dynamic_cast<MulticoreJobExecutorTTLAnswerMessage *>(message.get())) {
        return msg->ttl;
      } else {
        throw std::runtime_error("MulticoreJobExecutor::getTTL(): unexpected [" + msg->getName() + "] message");
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
    double MulticoreJobExecutor::getCoreFlopRate() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "floprate request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::getPrivateMailboxName();
      S4U_Mailbox::dput(this->mailbox_name,
                        new MulticoreJobExecutorFlopRateRequestMessage(
                                answer_mailbox,
                                this->getPropertyValueAsDouble(
                                        MulticoreJobExecutorProperty::FLOP_RATE_REQUEST_MESSAGE_PAYLOAD)));

      // Get the reply
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(answer_mailbox);
      if (MulticoreJobExecutorFlopRateAnswerMessage *msg = dynamic_cast<MulticoreJobExecutorFlopRateAnswerMessage *>(message.get())) {
        return msg->flop_rate;
      } else {
        throw std::runtime_error(
                "MulticoreJobExecutor::getCoreFLopRate(): unexpected [" + msg->getName() + "] message");
      }
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param plist: a property list
     */
    MulticoreJobExecutor::MulticoreJobExecutor(std::string hostname,
                                               bool supports_standard_jobs, bool supports_pilot_jobs,
                                               std::map<std::string, std::string> plist) :
            MulticoreJobExecutor::MulticoreJobExecutor(hostname,
                                                       supports_standard_jobs,
                                                       supports_pilot_jobs, plist, 0, -1, nullptr, "", nullptr) {

    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the job executor should be started
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param default_storage_service: a raw pointer to a StorageService object
     * @param plist: a property list
     */
    MulticoreJobExecutor::MulticoreJobExecutor(std::string hostname,
                                               bool supports_standard_jobs, bool supports_pilot_jobs,
                                               StorageService *default_storage_service,
                                               std::map<std::string, std::string> plist) :
            MulticoreJobExecutor::MulticoreJobExecutor(hostname,
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
     * @param num_worker_threads: the number of worker threads (i.e., sequential task executors)
     * @param ttl: the time-ti-live, in seconds
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     * @param default_storage_service: a raw pointer to a StorageService object
     *
     * @throw std::invalid_argument
     */
    MulticoreJobExecutor::MulticoreJobExecutor(
            std::string hostname,
            bool supports_standard_jobs,
            bool supports_pilot_jobs,
            std::map<std::string, std::string> plist,
            unsigned int num_worker_threads,
            double ttl,
            PilotJob *pj,
            std::string suffix,
            StorageService *default_storage_service) :
            ComputeService("multicore_job_executor" + suffix,
                           "multicore_job_executor" + suffix,
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
      this->num_worker_threads = num_worker_threads;
      this->ttl = ttl;
      this->has_ttl = (ttl >= 0);
      this->containing_pilot_job = pj;

      this->supports_standard_jobs = supports_standard_jobs;
      this->supports_pilot_jobs = supports_pilot_jobs;

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument &e) {
        throw &e;
      }
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int MulticoreJobExecutor::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_RED);

      /** Initialize all state **/
      initialize();

      this->death_date = -1.0;
      if (this->has_ttl) {
        this->death_date = S4U_Simulation::getClock() + this->ttl;
        WRENCH_INFO("Will be terminating at date %lf", this->death_date);
      }

      /** Main loop **/
      while (this->processNextMessage((this->has_ttl ? this->death_date - S4U_Simulation::getClock() : -1.0))) {

        // Clear pending asynchronous puts that are done
        S4U_Mailbox::clear_dputs();

        /** Dispatch currently pending tasks until no longer possible **/
        while (this->dispatchNextPendingTask());

        /** Dispatch jobs (and their tasks in the case of standard jobs) if possible) **/
        while (this->dispatchNextPendingJob());

      }

      WRENCH_INFO("Multicore Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     * @brief Dispatch one pending job, if possible
     * @return true if a job was dispatched, false otherwise
     */
    bool MulticoreJobExecutor::dispatchNextPendingJob() {

      /** If some idle core, then see if they can be used **/
      if ((this->busy_sequential_task_executors.size() < this->num_available_worker_threads)) {

        /** Look at pending jobs **/
        if (this->pending_jobs.size() > 0) {

          WorkflowJob *next_job = this->pending_jobs.front();

          switch (next_job->getType()) {
            case WorkflowJob::STANDARD: {
              StandardJob *job = (StandardJob *) next_job;

              // Put the job in the running queue
              this->pending_jobs.pop();
              this->running_jobs.insert(next_job);

              // Enqueue all its tasks in the task wait queue
              for (auto t : job->getTasks()) {
                this->pending_tasks.push(t);
              }

              // Try to dispatch its tasks if possible
              while (this->dispatchNextPendingTask());
              return true;
            }
            case WorkflowJob::PILOT: {
              PilotJob *job = (PilotJob *) next_job;
              WRENCH_INFO("Looking at dispatching pilot job %s with callbackmailbox %s",
                          job->getName().c_str(),
                          job->getCallbackMailbox().c_str());

              if (this->num_available_worker_threads - this->busy_sequential_task_executors.size() >=
                  job->getNumCores()) {

                // Immediately the number of available worker threads
                this->num_available_worker_threads -= job->getNumCores();
                WRENCH_INFO("Setting my number of available cores to %d", this->num_available_worker_threads);

                ComputeService *cs =
                        new MulticoreJobExecutor(S4U_Simulation::getHostName(),
                                                 true, false, this->property_list,
                                                 (unsigned int) job->getNumCores(),
                                                 job->getDuration(), job,
                                                 "_pilot",
                                                 this->default_storage_service);

                // Create and launch a compute service for the pilot job
                job->setComputeService(cs);

                // Put the job in the runnint queue
                this->pending_jobs.pop();
                this->running_jobs.insert(next_job);

                // Send the "Pilot job has started" callback
                // Note the getCallbackMailbox instead of the popCallbackMailbox, because
                // there will be another callback upon termination.
                S4U_Mailbox::dput(job->getCallbackMailbox(),
                                  new ComputeServicePilotJobStartedMessage(job, this, this->getPropertyValueAsDouble(
                                          MulticoreJobExecutorProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));

                // Push my own mailbox onto the pilot job!
                job->pushCallbackMailbox(this->mailbox_name);
                return true;
              }
              break;
            }
          }
        }
      }
      return false;
    }

    /**
     * @brief Dispatch one pending task to available worker threads (i.e., sequential task executors), if possible
     * @return true if a task was dispatched, false otherwise
     */
    bool MulticoreJobExecutor::dispatchNextPendingTask() {
      /** Dispatch tasks of currently running standard jobs to idle available worker threads **/
      if ((pending_tasks.size() > 0) &&
          (this->busy_sequential_task_executors.size() < this->num_available_worker_threads)) {

        // Get the first task out of the task wait queue
        WorkflowTask *to_run = pending_tasks.front();
        pending_tasks.pop();

        // Get the first idle sequential task executor and mark it as busy
        SequentialTaskExecutor *executor = *(this->idle_sequential_task_executors.begin());
        this->idle_sequential_task_executors.erase(executor);
        this->busy_sequential_task_executors.insert(executor);

        // Figure out the task's job
        StandardJob *job = (StandardJob *) (to_run->getJob());

        // Start the task on the sequential task executor
        WRENCH_INFO("Running task %s on one of my worker threads", to_run->getId().c_str());
        std::map<WorkflowFile *, StorageService *> task_file_locations;
        for (auto f : to_run->getInputFiles()) {
          if (job->getFileLocations().find(f) != job->getFileLocations().end()) {
            task_file_locations[f] = job->getFileLocations()[f];
          }
        }
        for (auto f : to_run->getOutputFiles()) {
          if (job->getFileLocations().find(f) != job->getFileLocations().end()) {
            task_file_locations[f] = job->getFileLocations()[f];
          }
        }

        executor->runTask(to_run, task_file_locations);

        // Put the task in the running task set
        this->running_tasks.insert(to_run);
        return true;

      } else {
        return false;
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
    bool MulticoreJobExecutor::processNextMessage(double timeout) {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message;

      // with a timeout?
      if (this->has_ttl) {
        if (timeout <= 0) {
          return false;
        } else {
          message = S4U_Mailbox::get(this->mailbox_name, timeout);
        }
      } else {
        message = S4U_Mailbox::get(this->mailbox_name);
      }

      // was there a timeout?
      if (message == nullptr) {
        WRENCH_INFO("Time out - must die.. !!");
        this->terminate();
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->terminate();
        // This is Synchronous
        S4U_Mailbox::put(msg->ack_mailbox,
                         new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                 MulticoreJobExecutorProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        return false;
      } else if (ComputeServiceSubmitStandardJobRequestMessage *msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        WRENCH_INFO("Asked to run a standard job with %ld tasks", msg->job->getNumTasks());
        if (!this->supportsStandardJobs()) {
          S4U_Mailbox::dput(msg->answer_mailbox,
                            new ComputeServiceSubmitStandardJobAnswerMessage(msg->job, this,
                                                               false,
                                                               new JobTypeNotSupported(msg->job, this),
                                                               this->getPropertyValueAsDouble(
                                                                       MulticoreJobExecutorProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
          return true;
        }

        this->pending_jobs.push(msg->job);
        S4U_Mailbox::dput(msg->answer_mailbox,
                          new ComputeServiceSubmitStandardJobAnswerMessage(msg->job, this,
                                                             true,
                                                             nullptr,
                                                             this->getPropertyValueAsDouble(
                                                                     MulticoreJobExecutorProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        return true;
      } else if (ComputeServiceSubmitPilotJobRequestMessage *msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        WRENCH_INFO("Asked to run a pilot job with %d cores for %lf seconds", msg->job->getNumCores(),
                    msg->job->getDuration());

        bool success = true;
        WorkflowExecutionFailureCause *failure_cause = nullptr;

        if (!this->supportsPilotJobs()) {
          S4U_Mailbox::dput(msg->answer_mailbox,
                            new ComputeServiceSubmitPilotJobAnswerMessage(msg->job,
                                                            this,
                                                            false,
                                                            new JobTypeNotSupported(msg->job, this),
                                                            this->getPropertyValueAsDouble(
                                                                    MulticoreJobExecutorProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          return true;
        }

        if (this->getNumCores() < msg->job->getNumCores()) {

          S4U_Mailbox::dput(msg->answer_mailbox,
                            new ComputeServiceSubmitPilotJobAnswerMessage(msg->job,
                                                            this,
                                                            false,
                                                            new NotEnoughCores(msg->job, this),
                                                            this->getPropertyValueAsDouble(
                                                                    MulticoreJobExecutorProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
          return true;
        }

        // success
        this->pending_jobs.push(msg->job);
        S4U_Mailbox::dput(msg->answer_mailbox,
                          new ComputeServiceSubmitPilotJobAnswerMessage(msg->job,
                                                          this,
                                                          true,
                                                          nullptr,
                                                          this->getPropertyValueAsDouble(
                                                                  MulticoreJobExecutorProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));

        return true;
      } else if (SequentialTaskExecutorTaskDoneMessage *msg = dynamic_cast<SequentialTaskExecutorTaskDoneMessage *>(message.get())) {

        processTaskCompletion(msg->task, msg->task_executor);
        return true;
      } else if (SequentialTaskExecutorTaskFailedMessage *msg = dynamic_cast<SequentialTaskExecutorTaskFailedMessage *>(message.get())) {
        processTaskFailure(msg->task, msg->task_executor, msg->cause);
        return true;
      } else if (ComputeServicePilotJobExpiredMessage *msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        processPilotJobCompletion(msg->job);
        return true;
      } else if (MulticoreJobExecutorNumCoresRequestMessage *msg = dynamic_cast<MulticoreJobExecutorNumCoresRequestMessage *>(message.get())) {
        MulticoreJobExecutorNumCoresAnswerMessage *answer_message = new MulticoreJobExecutorNumCoresAnswerMessage(this->num_worker_threads,
                                                                          this->getPropertyValueAsDouble(
                                                                                  MulticoreJobExecutorProperty::NUM_CORES_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dput(msg->answer_mailbox, answer_message);
        return true;
      } else if (MulticoreJobExecutorNumIdleCoresRequestMessage *msg = dynamic_cast<MulticoreJobExecutorNumIdleCoresRequestMessage *>(message.get())) {
        MulticoreJobExecutorNumIdleCoresAnswerMessage *answer_message = new MulticoreJobExecutorNumIdleCoresAnswerMessage(this->num_available_worker_threads,
                                                                                  this->getPropertyValueAsDouble(
                                                                                          MulticoreJobExecutorProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dput(msg->answer_mailbox, answer_message);
        return true;
      } else if (MulticoreJobExecutorTTLRequestMessage *msg = dynamic_cast<MulticoreJobExecutorTTLRequestMessage *>(message.get())) {
        MulticoreJobExecutorTTLAnswerMessage *answer_message = new MulticoreJobExecutorTTLAnswerMessage(this->death_date - S4U_Simulation::getClock(),
                                                                this->getPropertyValueAsDouble(
                                                                        MulticoreJobExecutorProperty::TTL_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dput(msg->answer_mailbox, answer_message);
        return true;
      } else if (MulticoreJobExecutorFlopRateRequestMessage *msg = dynamic_cast<MulticoreJobExecutorFlopRateRequestMessage *>(message.get())) {
        MulticoreJobExecutorFlopRateAnswerMessage *answer_message = new MulticoreJobExecutorFlopRateAnswerMessage(
                simgrid::s4u::Host::by_name(this->hostname)->getPstateSpeed(0),
                this->getPropertyValueAsDouble(
                        MulticoreJobExecutorProperty::FLOP_RATE_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dput(msg->answer_mailbox, answer_message);
        return true;
      } else {
        throw std::runtime_error("Unexpected [" + message->getName() + "] message");
      }
    }

/**
 * @brief Terminate all pilot job compute services
 */
    void MulticoreJobExecutor::terminateAllPilotJobs() {
      for (auto job : this->running_jobs) {
        if (job->getType() == WorkflowJob::PILOT) {
          PilotJob *pj = (PilotJob *) job;
          pj->getComputeService()->stop();
        }
      }
    }


/**
 * @brief Terminate (nicely or brutally) all worker threads (i.e., sequential task executors)
 */
    void MulticoreJobExecutor::terminateAllWorkerThreads() {
      // Kill all running sequential executors
      for (auto executor : this->busy_sequential_task_executors) {
        WRENCH_INFO("Brutally killing a busy sequential task executor");
        executor->kill();
      }

      // Cleanly terminate all idle sequential executors
      for (auto executor : this->idle_sequential_task_executors) {
        WRENCH_INFO("Cleanly stopping an idle sequential task executor");
        executor->stop();
      }
    }

    void MulticoreJobExecutor::failStandardJob(StandardJob *job, WorkflowExecutionFailureCause *cause) {
      WRENCH_INFO("Failing job %s", job->getName().c_str());
      // Set all tasks back to the READY state and wipe out output files
      for (auto failed_task: job->getTasks()) {
        failed_task->setReady();
        StorageService::deleteFiles(failed_task->getOutputFiles(), job->getFileLocations(),
                                    this->default_storage_service);

      }

      // Send back a job failed message
      WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
      // NOTE: This is synchronous so that the process doesn't fall off the end
      S4U_Mailbox::put(job->popCallbackMailbox(),
                       new ComputeServiceStandardJobFailedMessage(job, this, cause, this->getPropertyValueAsDouble(
                               MulticoreJobExecutorProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
    }

/**
 * @brief Declare all current jobs as failed (likely because the daemon is being terminated)
 */
    void MulticoreJobExecutor::failCurrentStandardJobs(WorkflowExecutionFailureCause *cause) {

      WRENCH_INFO("There are %ld pending jobs", this->pending_jobs.size());
      while (!this->pending_jobs.empty()) {
        WorkflowJob *workflow_job = this->pending_jobs.front();
        WRENCH_INFO("Failing job %s", workflow_job->getName().c_str());
        this->pending_jobs.pop();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          this->failStandardJob(job, cause);
        }
      }

      WRENCH_INFO("There are %ld running jobs", this->running_jobs.size());
      for (auto workflow_job : this->running_jobs) {
        WRENCH_INFO("Failing job %s", workflow_job->getName().c_str());
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          this->failStandardJob(job, cause);
        }
      }
    }

/**
 * @brief Initialize all state for the daemon
 */
    void MulticoreJobExecutor::initialize() {

      /* Start worker threads */

      // Figure out the number of worker threads
      if (this->num_worker_threads == 0) {
        this->num_worker_threads = (unsigned int) S4U_Simulation::getNumCores(S4U_Simulation::getHostName());
      }

      this->num_available_worker_threads = num_worker_threads;

      WRENCH_INFO("New Multicore Job Executor starting (%s) with %d worker threads ",
                  this->mailbox_name.c_str(), this->num_worker_threads);

      for (int i = 0; i < this->num_worker_threads; i++) {
        WRENCH_INFO("Starting a task executor on core #%d", i);
        std::unique_ptr<SequentialTaskExecutor> seq_executor =
                std::unique_ptr<SequentialTaskExecutor>(
                        new SequentialTaskExecutor(S4U_Simulation::getHostName(), this->mailbox_name,
                                                   this->default_storage_service,
                                                   this->simulation->getFileRegistryService(),
                                                   this->getPropertyValueAsDouble(
                                                           MulticoreJobExecutorProperty::TASK_STARTUP_OVERHEAD)));
        this->sequential_task_executors.push_back(std::move(seq_executor));
      }


      // Initialize the set of idle executors (cores)
      for (int i = 0; i < this->sequential_task_executors.size(); i++) {
        this->idle_sequential_task_executors.insert(this->sequential_task_executors[i].get());
      }

    }

/**
 * @brief Process a task completion
 *
 * @param task: the WorkflowTask that has completed
 * @param executor: a pointer to the worker thread (SequentialTaskExecutor) that has completed it
 */
    void MulticoreJobExecutor::processTaskCompletion(WorkflowTask *task, SequentialTaskExecutor *executor) {
      StandardJob *job = (StandardJob *) (task->getJob());
      WRENCH_INFO("One of my cores completed task %s (and its state is: %s", task->getId().c_str(), WorkflowTask::stateToString(task->getState()).c_str());

      task->setCompleted();

      // Remove the task from the running task queue
      this->running_tasks.erase(task);

      // Put that core's executor back into the pull of idle cores
      this->busy_sequential_task_executors.erase(executor);
      this->idle_sequential_task_executors.insert(executor);

      // Increase the "completed tasks" count of the job
      job->incrementNumCompletedTasks();

      // Generate a SimulationTimestamp
      this->simulation->output.addTimestamp<SimulationTimestampTaskCompletion>(
              new SimulationTimestampTaskCompletion(task));

      // Send the callback to the originator if necessary and remove the job from
      // the list of pending jobs
      if (job->getNumCompletedTasks() == job->getNumTasks()) {
        this->running_jobs.erase(job);
        S4U_Mailbox::dput(job->popCallbackMailbox(),
                          new ComputeServiceStandardJobDoneMessage(job, this, this->getPropertyValueAsDouble(
                                  MulticoreJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      }
    }

/**
 * @brief Process a task failure
 *
 * @param task: the workflow task that has failed
 * @param executor: the worker thread (SequentialTaskExecutor) on which the failure occurred
 * @param cause: the cause of the failure
 */
    void MulticoreJobExecutor::processTaskFailure(WorkflowTask *task,
                                                  SequentialTaskExecutor *executor,
                                                  WorkflowExecutionFailureCause *cause) {

      // Put that core's executor back into the pull of idle cores
      this->busy_sequential_task_executors.erase(executor);
      this->idle_sequential_task_executors.insert(executor);

      // Get the job for the task
      StandardJob *job = (StandardJob *) (task->getJob());
      WRENCH_INFO("One of my cores has failed to run task %s", task->getId().c_str());

      // Remove the job from the list of running jobs
      this->running_jobs.erase(job);

      // Fail the job
      this->failStandardJob(job, cause);

    }

/**
 * @brief Terminate the daemon, dealing with pending/running jobs
 */
    void MulticoreJobExecutor::terminate() {

      this->setStateToDown();

      WRENCH_INFO("Terminate all worker threads");
      this->terminateAllWorkerThreads();

      WRENCH_INFO("Failing current standard jobs");
      this->failCurrentStandardJobs(new ServiceIsDown(this));

      WRENCH_INFO("Terminate all pilot jobs");
      this->terminateAllPilotJobs();

      // Am I myself a pilot job?
      if (this->containing_pilot_job) {

        WRENCH_INFO("Letting the level above that the pilot job has ended on mailbox %s",
                    this->containing_pilot_job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        S4U_Mailbox::put(this->containing_pilot_job->popCallbackMailbox(),
                         new ComputeServicePilotJobExpiredMessage(this->containing_pilot_job, this,
                                                    this->getPropertyValueAsDouble(
                                                            MulticoreJobExecutorProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));

      }
    }

/**
 * @brief Process a pilot job completion
 *
 * @param job: pointer to the PilotJob object
 */
    void MulticoreJobExecutor::processPilotJobCompletion(PilotJob *job) {

      // Remove the job from the running list
      this->running_jobs.erase(job);

      // Update the number of available cores
      this->num_available_worker_threads += job->getNumCores();

      // Forward the notification
      S4U_Mailbox::dput(job->popCallbackMailbox(),
                        new ComputeServicePilotJobExpiredMessage(job, this,
                                                   this->getPropertyValueAsDouble(
                                                           MulticoreJobExecutorProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));

      return;
    }

};