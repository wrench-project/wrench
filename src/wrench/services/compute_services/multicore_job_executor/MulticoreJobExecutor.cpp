/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <simulation/Simulation.h>
#include <logging/TerminalOutput.h>
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "exceptions/ComputeServiceIsDownException.h"
#include "workflow_job/StandardJob.h"
#include "workflow_job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_job_executor, "Log category for Multicore Job Executor");


namespace wrench {

    /**
     * @brief Stop the service
     *
     * @throw std::runtime_error
     */
    void MulticoreJobExecutor::stop() {

      this->state = ComputeService::DOWN;

      WRENCH_INFO("Telling the daemon listening on (%s) to terminate", this->mailbox_name.c_str());
      // Send a termination message to the daemon's mailbox - SYNCHRONOUSLY
      std::string ack_mailbox = this->mailbox_name + "_kill";
      S4U_Mailbox::put(this->mailbox_name,
                       new StopDaemonMessage(
                               ack_mailbox,
                               this->getPropertyValueAsDouble(STOP_DAEMON_MESSAGE_PAYLOAD)));
      // Wait for the ack
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(ack_mailbox);
      if (message->type != SimulationMessage::Type::DAEMON_STOPPED) {
        throw std::runtime_error("Wrong message type received while expecting DAEMON_STOPPED");
      }
    }


    /**
     * @brief Have the service execute a standard job
     *
     * @param job: a pointer a StandardJob object
     *
     * @throw ComputeServiceIsDownException
     */
    void MulticoreJobExecutor::runStandardJob(StandardJob *job) {

      if (this->state == ComputeService::DOWN) {
        throw ComputeServiceIsDownException(this->getName());
      }

      // Synchronously send a "run a task" message to the daemon's mailbox
      S4U_Mailbox::dput(this->mailbox_name, new RunStandardJobMessage(job, this->getPropertyValueAsDouble(
              RUN_STANDARD_JOB_MESSAGE_PAYLOAD)));
    };

    /**
     * @brief Have the service execute a pilot job
     *
     * @param task: a pointer the PilotJob object
     * @param callback_mailbox: the name of a mailbox to which a "pilot job started" callback will be sent
     *
     * @throw ComputeServiceIsDownException
     */
    void MulticoreJobExecutor::runPilotJob(PilotJob *job) {

      if (this->state == ComputeService::DOWN) {
        throw ComputeServiceIsDownException(this->getName());
      }

      //  send a "run a task" message to the daemon's mailbox
      S4U_Mailbox::dput(this->mailbox_name,
                        new RunPilotJobMessage(job, this->getPropertyValueAsDouble(RUN_PILOT_JOB_MESSAGE_PAYLOAD)));
    };

    /**
     * @brief Finds out how many  cores the  service has
     *
     * @return the number of cores
     *
     * @throw ComputeServiceIsDownException
     */
    unsigned long MulticoreJobExecutor::getNumCores() {

      if (this->state == ComputeService::DOWN) {
        throw ComputeServiceIsDownException(this->getName());
      }

      return (unsigned long) S4U_Simulation::getNumCores(this->hostname);
    }

    /**
     * @brief Finds out how many idle cores the  service has
     *
     * @return the number of currently idle cores
     *
     * @throw ComputeServiceIsDownException
     */
    unsigned long MulticoreJobExecutor::getNumIdleCores() {

      if (this->state == ComputeService::DOWN) {
        throw ComputeServiceIsDownException(this->getName());
      }

      S4U_Mailbox::dput(this->mailbox_name, new NumIdleCoresRequestMessage(
              this->getPropertyValueAsDouble(NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD)));
      std::unique_ptr<SimulationMessage> msg = S4U_Mailbox::get(this->mailbox_name + "_answers");
      std::unique_ptr<NumIdleCoresAnswerMessage> m(static_cast<NumIdleCoresAnswerMessage *>(msg.release()));
      return m->num_idle_cores;
    }

    /**
     * @brief Finds out the TTL
     *
     * @return the TTL in seconds
     *
     * @throw ComputeServiceIsDownException
     */
    double MulticoreJobExecutor::getTTL() {

      if (this->state == ComputeService::DOWN) {
        throw ComputeServiceIsDownException(this->getName());
      }

      S4U_Mailbox::dput(this->mailbox_name,
                        new TTLRequestMessage(this->getPropertyValueAsDouble(TTL_REQUEST_MESSAGE_PAYLOAD)));
      std::unique_ptr<SimulationMessage> msg = S4U_Mailbox::get(this->mailbox_name + "_answers");
      std::unique_ptr<TTLAnswerMessage> m(static_cast<TTLAnswerMessage *>(msg.release()));
      return m->ttl;
    }

    /**
     * @brief Finds out the flop rate of the cores
     *
     * @return the clock rate in Flops/sec
     *
     * @throw ComputeServiceIsDownException
     */
    double MulticoreJobExecutor::getCoreFlopRate() {

      if (this->state == ComputeService::DOWN) {
        throw ComputeServiceIsDownException(this->getName());
      }
      return simgrid::s4u::Host::by_name(this->hostname)->getPstateSpeed(0);
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
                                               std::map<MulticoreJobExecutor::Property, std::string> plist) :
            MulticoreJobExecutor::MulticoreJobExecutor(hostname, plist, 0, -1, nullptr, "") {

      // Set the supported jobs
      this->setSupportStandardJobs(supports_standard_jobs);
      this->setSupportPilotJobs(supports_pilot_jobs);
    }


    /**
     * @brief Constructor that starts the daemon for the service on a host,
     *        registering it with a WRENCH Simulation
     *
     * @param hostname: the name of the host
     * @param plist: a property list
     * @param num_worker_threads: the number of worker threads (i.e., sequential task executors)
     * @param ttl: the time-ti-live, in seconds
     * @param pj: a containing PilotJob  (nullptr if none)
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    MulticoreJobExecutor::MulticoreJobExecutor(
            std::string hostname,
            std::map<MulticoreJobExecutor::Property, std::string> plist,
            unsigned int num_worker_threads,
            double ttl,
            PilotJob *pj,
            std::string suffix) :
            ComputeService("multicore_job_executor"),
            S4U_DaemonWithMailbox("multicore_job_executor" + suffix, "multicore_job_executor" + suffix) {

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

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument e) {
        throw e;
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


//			if (this->containing_pilot_job) {
//				WRENCH_INFO("MY CONTAINING PILOT JOB HAS CALLBACK MAILBOX '%s'", this->containing_pilot_job->getCallbackMailbox().c_str());
//			}

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

                // Create and launch a compute service (same properties as mine)
                ComputeService *cs =
                        Simulation::createUnregisteredMulticoreJobExecutor(S4U_Simulation::getHostName(),
                                                                           true, false,
                                                                           this->property_list,
                                                                           job->getNumCores(),
                                                                           job->getDuration(),
                                                                           job,
                                                                           "_pilot");

                // Create and launch a compute service for the pilot job
                job->setComputeService(cs);

                // Put the job in the runnint queue
                this->pending_jobs.pop();
                this->running_jobs.insert(next_job);

                // Send the "Pilot job has started" callback
                // Note the getCallbackMailbox instead of the popCallbackMailbox, because
                // there will be another callback upon termination.
                S4U_Mailbox::dput(job->getCallbackMailbox(),
                                  new PilotJobStartedMessage(job, this, this->getPropertyValueAsDouble(
                                          PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));

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

        // Start the task on the sequential task executor
        WRENCH_INFO("Running task %s on one of my worker threads", to_run->getId().c_str());
        executor->runTask(to_run);

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

      WRENCH_INFO("Got a [%s] message", message->toString().c_str());

      switch (message->type) {

        case SimulationMessage::STOP_DAEMON: {
          std::unique_ptr<StopDaemonMessage> m(static_cast<StopDaemonMessage *>(message.release()));

          this->terminate();
          // This is Synchronous
          S4U_Mailbox::put(m->ack_mailbox,
                           new DaemonStoppedMessage(this->getPropertyValueAsDouble(DAEMON_STOPPED_MESSAGE_PAYLOAD)));
          return false;
        }

        case SimulationMessage::RUN_STANDARD_JOB: {
          std::unique_ptr<RunStandardJobMessage> m(static_cast<RunStandardJobMessage *>(message.release()));
          WRENCH_INFO("Asked to run a standard job with %ld tasks", m->job->getNumTasks());
          if (!this->supportsStandardJobs()) {
            S4U_Mailbox::dput(m->job->popCallbackMailbox(), new JobTypeNotSupportedMessage(m->job, this,
                                                                                           this->getPropertyValueAsDouble(
                                                                                                   JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD)));
          } else {
            this->pending_jobs.push(m->job);
          }
          return true;
        }

        case SimulationMessage::RUN_PILOT_JOB: {
          std::unique_ptr<RunPilotJobMessage> m(static_cast<RunPilotJobMessage *>(message.release()));
          WRENCH_INFO("Asked to run a pilot job with %d cores for %lf seconds", m->job->getNumCores(),
                      m->job->getDuration());
          if (!this->supportsPilotJobs()) {
            S4U_Mailbox::dput(m->job->popCallbackMailbox(), new JobTypeNotSupportedMessage(m->job, this,
                                                                                           this->getPropertyValueAsDouble(
                                                                                                   JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD)));
            return true;
          }
          if (this->getNumCores() < m->job->getNumCores()) {
            S4U_Mailbox::dput(m->job->popCallbackMailbox(), new NotEnoughCoresMessage(m->job, this,
                                                                                      this->getPropertyValueAsDouble(
                                                                                              NOT_ENOUGH_CORES_MESSAGE_PAYLOAD)));
            return true;
          }
          this->pending_jobs.push(m->job);
          return true;
        }

        case SimulationMessage::TASK_DONE: {
          std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage *>(message.release()));
          processTaskCompletion(m->task, m->task_executor);
          return true;
        }

        case SimulationMessage::PILOT_JOB_EXPIRED: {
          std::unique_ptr<PilotJobExpiredMessage> m(static_cast<PilotJobExpiredMessage *>(message.release()));
          processPilotJobCompletion(m->job);
          return true;
        }

        case SimulationMessage::NUM_IDLE_CORES_REQUEST: {
          std::unique_ptr<NumIdleCoresRequestMessage> m(static_cast<NumIdleCoresRequestMessage *>(message.release()));
          NumIdleCoresAnswerMessage *msg = new NumIdleCoresAnswerMessage(this->num_available_worker_threads,
                                                                         this->getPropertyValueAsDouble(
                                                                                 NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD));
          S4U_Mailbox::dput(this->mailbox_name + "_answers", msg);
          return true;
        }

        case SimulationMessage::TTL_REQUEST: {
          std::unique_ptr<TTLRequestMessage> m(static_cast<TTLRequestMessage *>(message.release()));
          TTLAnswerMessage *msg = new TTLAnswerMessage(this->death_date - S4U_Simulation::getClock(),
                                                       this->getPropertyValueAsDouble(TTL_ANSWER_MESSAGE_PAYLOAD));
          S4U_Mailbox::dput(this->mailbox_name + "_answers", msg);
          return true;
        }

        default: {
          throw std::runtime_error("Unknown message type: " + std::to_string(message->type));
        }
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

    /**
     * @brief Declare all current jobs as failed (liekly because the daemon is being terminated)
     */
    void MulticoreJobExecutor::failCurrentStandardJobs() {

      WRENCH_INFO("There are %ld pending jobs", this->pending_jobs.size());
      while (!this->pending_jobs.empty()) {
        WorkflowJob *workflow_job = this->pending_jobs.front();
        WRENCH_INFO("Failing job %s", workflow_job->getName().c_str());
        this->pending_jobs.pop();
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          // Set all tasks back to the READY state
          for (auto failed_task: ((StandardJob *) job)->getTasks()) {
            failed_task->setReady();
          }
          // Send back a job failed message
          WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
          // NOTE: This is synchronous so that the process doesn't fall off the end
          S4U_Mailbox::put(job->popCallbackMailbox(),
                           new StandardJobFailedMessage(job, this, this->getPropertyValueAsDouble(
                                   STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        }
      }

      WRENCH_INFO("There are %ld running jobs", this->running_jobs.size());
      for (auto workflow_job : this->running_jobs) {
        WRENCH_INFO("Failing job %s", workflow_job->getName().c_str());
        if (workflow_job->getType() == WorkflowJob::STANDARD) {
          StandardJob *job = (StandardJob *) workflow_job;
          // Set all tasks back to the READY state
          for (auto failed_task: ((StandardJob *) job)->getTasks()) {
            failed_task->setReady();
          }
          // Send back a job failed message
          WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
          // NOTE: This is synchronous so that the process doesn't fall off the end
          S4U_Mailbox::put(job->popCallbackMailbox(),
                           new StandardJobFailedMessage(job, this, this->getPropertyValueAsDouble(
                                   STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
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
                                                   this->getPropertyValueAsDouble(
                                                           MulticoreJobExecutor::Property::TASK_STARTUP_OVERHEAD)));
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
      WRENCH_INFO("One of my cores completed task %s", task->getId().c_str());

      // Remove the task from the running task queue
      this->running_tasks.erase(task);

      // Put that core's executor back into the pull of idle cores
      this->busy_sequential_task_executors.erase(executor);
      this->idle_sequential_task_executors.insert(executor);

      // Increase the "completed tasks" count of the job
      job->incrementNumCompletedTasks();

      // Generate a SimulationTimestamp
      this->simulation->output.addTimestamp<SimulationTimestampTaskCompletion>(new SimulationTimestampTaskCompletion(task));

      // Send the callback to the originator if necessary and remove the job from
      // the list of pending jobs
      if (job->getNumCompletedTasks() == job->getNumTasks()) {
        this->running_jobs.erase(job);
        S4U_Mailbox::dput(job->popCallbackMailbox(),
                          new StandardJobDoneMessage(job, this, this->getPropertyValueAsDouble(
                                  STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      }
    }

    /**
     * @brief Terminate the daemon, dealing with pending/running jobs
     */
    void MulticoreJobExecutor::terminate() {

      this->setStateToDown();

      WRENCH_INFO("Terminate all worker threads");
      this->terminateAllWorkerThreads();

      WRENCH_INFO("Failing current standard jobs");
      this->failCurrentStandardJobs();

      WRENCH_INFO("Terminate all pilot jobs");
      this->terminateAllPilotJobs();

      // Am I myself a pilot job?
      if (this->containing_pilot_job) {

        WRENCH_INFO("Letting the level above that the pilot job has ended on mailbox %s",
                    this->containing_pilot_job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        S4U_Mailbox::put(this->containing_pilot_job->popCallbackMailbox(),
                         new PilotJobExpiredMessage(this->containing_pilot_job, this,
                                                    this->getPropertyValueAsDouble(PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));

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
                        new PilotJobExpiredMessage(job, this,
                                                   this->getPropertyValueAsDouble(PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));

      return;
    }

};