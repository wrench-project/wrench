/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <logging/TerminalOutput.h>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simgrid_S4U_util/S4U_Simulation.h"
#include "JobManager.h"
#include "workflow_job/StandardJob.h"
#include "workflow_job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(job_manager, "Log category for Job Manager");


namespace wrench {

    /**
     * @brief Constructor, which starts a job manager daemon
     *
     * @param workflow: a pointer to the Workflow whose jobs are to be managed
     */
    JobManager::JobManager(Workflow *workflow) :
            S4U_DaemonWithMailbox("job_manager", "job_manager") {

      this->workflow = workflow;

      // Start the daemon
      std::string localhost = S4U_Simulation::getHostName();
      this->start(localhost);
    }

    /**
     * @brief Destructor, which kills the daemon
     */
    JobManager::~JobManager() {
      this->kill();
    }

    /**
     * @brief Kill the job manager (brutally terminate the damone, clears all jobs)
     */
    void JobManager::kill() {
      this->kill_actor();
      this->jobs.clear();
    }

    /**
     * @brief Stop the job manager
     */
    void JobManager::stop() {
      S4U_Mailbox::put(this->mailbox_name, new StopDaemonMessage("", 0.0));
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a vector of WorkflowTask pointers to include in the StandardJob
     *
     * @return a raw pointer to the StandardJob
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks) {
      StandardJob *raw_ptr = new StandardJob(tasks);
      std::unique_ptr<WorkflowJob> job = std::unique_ptr<StandardJob>(raw_ptr);

      this->jobs[job->getName()] = std::move(job);
      return raw_ptr;
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a pointer the single WorkflowTask to include in the StandardJob
     *
     * @return a raw pointer to the StandardJob
     */
    StandardJob *JobManager::createStandardJob(WorkflowTask *task) {
      std::vector<WorkflowTask *> tasks;
      tasks.push_back(task);
      return this->createStandardJob(tasks);
    }

    /**
     * @brief Create a pilot job
     *
     * @param workflow: a pointer to a Workflow
     * @param num_cores: the number of cores required by the PilotJob
     * @param duration: the PilotJob duration in seconds
     * @return a raw pointer to the PilotJob
     */
    PilotJob *JobManager::createPilotJob(Workflow *workflow, int num_cores, double duration) {
      PilotJob *raw_ptr = new PilotJob(workflow, num_cores, duration);
      std::unique_ptr<WorkflowJob> job = std::unique_ptr<PilotJob>(raw_ptr);
      this->jobs[job->getName()] = std::move(job);
      return raw_ptr;
    }

    /**
     * @brief Submit a job to a compute service
     *
     * @param job: a pointer to a WorkflowJob object
     * @param compute_service: a pointer to a ComputeService object
     */
    void JobManager::submitJob(WorkflowJob *job, ComputeService *compute_service) {

      // Push back the mailbox of the manager,
      // so that it will get the initial callback
      job->pushCallbackMailbox(this->mailbox_name);

      // Update the job state and insert it into the pending list
      switch (job->getType()) {
        case WorkflowJob::STANDARD: {
          ((StandardJob *) job)->state = StandardJob::PENDING;
          for (auto t : ((StandardJob *) job)->tasks) {
            t->setState(WorkflowTask::State::PENDING);
          }
          this->pending_standard_jobs.insert((StandardJob *) job);
          break;
        }
        case WorkflowJob::PILOT: {
          ((PilotJob *) job)->state = PilotJob::PENDING;
          this->pending_pilot_jobs.insert((PilotJob *) job);
          break;
        }
      }

      // Submit the job to the service
      compute_service->runJob(job);

    }

    /**
     * @brief Cancel a PilotJob that hasn't expired yet
     * @param job: a pointer to the PilotJob
     */
    void JobManager::cancelPilotJob(PilotJob *job) {
      throw std::runtime_error("cancelPilotJob() not implemented yet");
    }

    /**
     * @brief Get the list of currently running PilotJob instances
     * @return a set of PilotJob pointers
     */
    std::set<PilotJob *> JobManager::getRunningPilotJobs() {
      return this->running_pilot_jobs;
    }

    /**
     * @brief Get the list of currently pending PilotJob instances
     * @return a set of PilotJob pointers
     */
    std::set<PilotJob *> JobManager::getPendingPilotJobs() {
      return this->pending_pilot_jobs;
    }

    /**
     * @brief Forget a job (to free memory, typically once the job is completed)
     *
     * @param job: a pointer to a WorkflowJob
     */
    void JobManager::forgetJob(WorkflowJob *) {
      throw std::runtime_error("forgetJob() not implemented yet");
    }


    /**
     * @brief Main method of the daemon that implements the JobManager
     * @return 0 in success
     */
    int JobManager::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_YELLOW);

      WRENCH_INFO("New Job Manager starting (%s)", this->mailbox_name.c_str());

      bool keep_going = true;
      while (keep_going) {
        std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

        // Clear finished asynchronous dput()
        S4U_Mailbox::clear_dputs();

        WRENCH_INFO("Job Manager got a %s message", message->toString().c_str());
        switch (message->type) {

          case SimulationMessage::STOP_DAEMON: {
            // There shouldn't be any need to clean any state up
            keep_going = false;
            break;
          }

          case SimulationMessage::JOB_TYPE_NOT_SUPPORTED: {
            std::unique_ptr<JobTypeNotSupportedMessage> m(static_cast<JobTypeNotSupportedMessage *>(message.release()));

            // update job state and remove from list
            if (m->job->getType() == WorkflowJob::STANDARD) {
              StandardJob *job = (StandardJob *) m->job;
              job->state = StandardJob::State::NOT_SUBMITTED;
              this->pending_standard_jobs.erase(job);

              // update the task states
              for (auto t: job->getTasks()) {
                t->setState(WorkflowTask::State::READY);
              }
            }

            if (m->job->getType() == WorkflowJob::STANDARD) {
              PilotJob *job = (PilotJob *) m->job;
              job->state = PilotJob::State::NOT_SUBMITTED;
              this->pending_pilot_jobs.erase(job);
            }

            // Forward the notification along the notification chain
            S4U_Mailbox::dput(m->job->popCallbackMailbox(),
                              new JobTypeNotSupportedMessage(m->job, m->compute_service, 0));
            break;
          }

          case SimulationMessage::STANDARD_JOB_DONE: {
            std::unique_ptr<StandardJobDoneMessage> m(static_cast<StandardJobDoneMessage *>(message.release()));

            // update job state
            StandardJob *job = m->job;
            job->state = StandardJob::State::COMPLETED;

            // move the job from the "pending" list to the "completed" list
            this->pending_standard_jobs.erase(job);
            this->completed_standard_jobs.insert(job);

            // Forward the notification along the notification chain
            S4U_Mailbox::dput(job->popCallbackMailbox(),
                              new StandardJobDoneMessage(job, m->compute_service, 0.0));
            break;
          }

          case SimulationMessage::STANDARD_JOB_FAILED: {
            std::unique_ptr<StandardJobFailedMessage> m(static_cast<StandardJobFailedMessage *>(message.release()));

            // update job state
            StandardJob *job = m->job;
            job->state = StandardJob::State::FAILED;

            // update the task states
            for (auto t: job->getTasks()) {
              t->incrementFailureCount();
              t->setState(WorkflowTask::State::READY);
            }

            // remove the job from the "pending" list
            this->pending_standard_jobs.erase(job);

            // Forward the notification along the notification chain
            S4U_Mailbox::dput(job->popCallbackMailbox(),
                              new StandardJobFailedMessage(job, m->compute_service, 0.0));
            break;
          }

          case SimulationMessage::PILOT_JOB_STARTED: {
            std::unique_ptr<PilotJobStartedMessage> m(static_cast<PilotJobStartedMessage *>(message.release()));

            // update job state
            PilotJob *job = m->job;
            job->state = PilotJob::State::RUNNING;

            // move the job from the "pending" list to the "running" list
            this->pending_pilot_jobs.erase(job);
            this->running_pilot_jobs.insert(job);

            // Forward the notification to the source
            WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
            S4U_Mailbox::dput(job->getOriginCallbackMailbox(),
                              new PilotJobStartedMessage(job, m->compute_service, 0.0));

            break;
          }

          case SimulationMessage::PILOT_JOB_EXPIRED: {
            std::unique_ptr<PilotJobExpiredMessage> m(static_cast<PilotJobExpiredMessage *>(message.release()));

            // update job state
            PilotJob *job = m->job;
            job->state = PilotJob::State::EXPIRED;

            // Remove the job from the "running" list
            this->running_pilot_jobs.erase(job);
            WRENCH_INFO("THERE ARE NOW %ld running pilot jobs", this->running_pilot_jobs.size());

            // Forward the notification to the source
            WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
            S4U_Mailbox::dput(job->getOriginCallbackMailbox(),
                              new PilotJobExpiredMessage(job, m->compute_service, 0.0));

            break;
          }

          default: {
            throw std::runtime_error("Invalid message type " + std::to_string(message->type));
          }
        }

      }

      WRENCH_INFO("New Multicore Task Executor terminating");
      return 0;
    }

};
