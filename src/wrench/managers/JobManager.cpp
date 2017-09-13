/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/JobManager.h"
#include "wrench/services/compute/ComputeService.h"
#include "services/ServiceMessage.h"
#include "services/compute/ComputeServiceMessage.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "simulation/SimulationMessage.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/workflow/job/StandardJob.h"
#include "workflow/job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(job_manager, "Log category for Job Manager");

namespace wrench {

    /**
     * @brief Constructor, which starts a job manager daemon
     *
     * @param workflow: the workflow whose jobs are to be managed
     */
    JobManager::JobManager(Workflow *workflow) :
            S4U_Daemon("job_manager", "job_manager") {

      this->workflow = workflow;

      // Start the daemon
      std::string localhost = S4U_Simulation::getHostName();
      this->start(localhost);
    }

    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    JobManager::~JobManager() {
      this->kill();
      this->jobs.clear();
    }

    /**
     * @brief Kill the job manager (brutally terminate the daemon, clears all jobs)
     */
    void JobManager::kill() {
      this->kill_actor();
      this->jobs.clear();
    }

    /**
     * @brief Stop the job manager
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void JobManager::stop() {
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
      } catch (std::shared_ptr<NetworkError> cause) {
        throw WorkflowExecutionException(cause);
      }
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a vector of tasks
     * @param file_locations: a map that specifies on which storage services input/output files should be read/written
     *         (default storage is used otherwise, provided that the job is submitted to a compute service
     *          for which that default was specified)
     * @param pre_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         before task executions begin
     * @param post_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         after task executions end
     * @param cleanup_file_deletions: a set of file tuples that specify file deletion operations that should be completed
     *                                at the end of the job
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks,
                                               std::map<WorkflowFile *, StorageService *> file_locations,
                                               std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                                               std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                                               std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions) {
      if (tasks.size() < 1) {
        throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
      }

      // Do a sanity check of everything (looking for nullptr)
      for (auto t : tasks) {
        if (t == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr task in the task vector");
        }
      }

      for (auto fl : file_locations) {
        if (fl.first == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr workflow file in the file_locations map");
        }
        if (fl.second == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr storage service in the file_locations map");
        }
      }

      for (auto fc : pre_file_copies) {
        if (std::get<0>(fc) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr workflow file in the pre_file_copies set");
        }
        if (std::get<1>(fc) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr storage service in the pre_file_copies set");
        }
        if (std::get<2>(fc) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr storage service in the pre_file_copies set");
        }
      }

      for (auto fc : post_file_copies) {
        if (std::get<0>(fc) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr workflow file in the post_file_copies set");
        }
        if (std::get<1>(fc) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr storage service in the post_file_copies set");
        }
        if (std::get<2>(fc) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr storage service in the post_file_copies set");
        }
      }

      for (auto fd : cleanup_file_deletions) {
        if (std::get<0>(fd) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr workflow file in the cleanup_file_deletions set");
        }
        if (std::get<1>(fd) == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr storage service in the cleanup_file_deletions set");
        }
      }

      StandardJob *raw_ptr = new StandardJob(tasks, file_locations, pre_file_copies, post_file_copies,
                                             cleanup_file_deletions);
      std::unique_ptr<WorkflowJob> job = std::unique_ptr<StandardJob>(raw_ptr);

      this->jobs[raw_ptr] = std::move(job);
      return raw_ptr;
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a vector of tasks
     * @param file_locations: a map that specifies on which storage services input/output files should be read/written
     *         (default storage is used otherwise, provided that the job is submitted to a compute service
     *          for which that default was specified)
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks,
                                               std::map<WorkflowFile *, StorageService *> file_locations) {
      if (tasks.size() < 1) {
        throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
      }

      return this->createStandardJob(tasks, file_locations, {}, {}, {});
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a task
     * @param file_locations: a map that specifies on which storage services input/output files should be read/written
     *         (default storage is used otherwise, provided that the job is submitted to a compute service
     *          for which that default was specified)
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *
    JobManager::createStandardJob(WorkflowTask *task, std::map<WorkflowFile *, StorageService *> file_locations) {

      if (task == nullptr) {
        throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
      }

      std::vector<WorkflowTask *> tasks;
      tasks.push_back(task);
      return this->createStandardJob(tasks, file_locations);
    }

    /**
     * @brief Create a pilot job
     *
     * @param workflow: a workflow
     * @param num_cores: the number of cores required by the pilot job
     * @param duration: the pilot job's duration in seconds
     * @return athe pilot job
     *
     * @throw std::invalid_argument
     */
    PilotJob *JobManager::createPilotJob(Workflow *workflow, unsigned int num_cores, double duration) {
      if ((workflow == nullptr) || (num_cores < 1) || (duration <= 0.0)) {
        throw std::invalid_argument("JobManager::createPilotJob(): Invalid arguments");
      }
      PilotJob *raw_ptr = new PilotJob(workflow, num_cores, duration);
      std::unique_ptr<WorkflowJob> job = std::unique_ptr<PilotJob>(raw_ptr);
      this->jobs[raw_ptr] = std::move(job);
      return raw_ptr;
    }

    /**
     * @brief Submit a job to a compute service
     *
     * @param job: a workflow job
     * @param compute_service: a compute service
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void JobManager::submitJob(WorkflowJob *job, ComputeService *compute_service) {

      if ((job == nullptr) || (compute_service == nullptr)) {
        throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
      }

      // Push back the mailbox of the manager,
      // so that it will getMessage the initial callback
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
      try {
        compute_service->runJob(job);
        job->setParentComputeService(compute_service);
      } catch (WorkflowExecutionException &e) {
        throw;
      }

    }

    /**
     * @brief Terminate a job (standard or pilot) that hasn't completed/expired/failed yet
     * @param job: the job
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void JobManager::terminateJob(WorkflowJob *job) {
      if (job == nullptr) {
        throw std::invalid_argument("JobManager::terminateJob(): invalid argument");
      }

      if (job->getParentComputeService() == nullptr) {
        throw WorkflowExecutionException(new JobCannotBeTerminated(job));
      }

      try {
        job->getParentComputeService()->terminateJob(job);
      } catch (std::exception &e) {
        throw;
      }

    }

    /**
     * @brief Get the list of currently running pilot jobs
     * @return a set of pilot jobs
     */
    std::set<PilotJob *> JobManager::getRunningPilotJobs() {
      return this->running_pilot_jobs;
    }

    /**
     * @brief Get the list of currently pending pilot jobs
     * @return a set of pilot jobs
     */
    std::set<PilotJob *> JobManager::getPendingPilotJobs() {
      return this->pending_pilot_jobs;
    }

    /**
     * @brief Forget a job (to free memory, only once a job is completed)
     *
     * @param job: a workflow job
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void JobManager::forgetJob(WorkflowJob *job) {
      if (job == nullptr) {
        throw std::invalid_argument("JobManager::forgetJob(): invalid argument");
      }

      if (job->getType() == WorkflowJob::STANDARD) {
        if ((this->pending_standard_jobs.find((StandardJob *) job) != this->pending_standard_jobs.end()) ||
            (this->running_standard_jobs.find((StandardJob *) job) != this->running_standard_jobs.end())) {
          throw WorkflowExecutionException(new JobCannotBeForgotten(job));
        }
        if (this->completed_standard_jobs.find((StandardJob *) job) != this->completed_standard_jobs.end()) {
          this->completed_standard_jobs.erase((StandardJob *) job);
          this->jobs.erase(job);
          return;
        }
        if (this->failed_standard_jobs.find((StandardJob *) job) != this->failed_standard_jobs.end()) {
          this->failed_standard_jobs.erase((StandardJob *) job);
          this->jobs.erase(job);
          return;
        }
        throw std::invalid_argument("JobManager::forgetJob(): unknown standard job");
      }

      if (job->getType() == WorkflowJob::PILOT) {
        if ((this->pending_pilot_jobs.find((PilotJob *) job) != this->pending_pilot_jobs.end()) ||
            (this->running_pilot_jobs.find((PilotJob *) job) != this->running_pilot_jobs.end())) {
          throw WorkflowExecutionException(new JobCannotBeForgotten(job));
        }
        if (this->completed_pilot_jobs.find((PilotJob *) job) != this->completed_pilot_jobs.end()) {
          this->jobs.erase(job);
        }
        throw std::invalid_argument("JobManager::forgetJob(): unknown pilot job");
      }

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
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
          message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> cause) {
          continue;
        }

        if (message == nullptr) {
          WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
          break;
        }
        // Clear finished asynchronous dputMessage()
//        S4U_Mailbox::clear_dputs();

        WRENCH_INFO("Job Manager got a %s message", message->getName().c_str());

        if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
          // There shouldn't be any need to clean any state up
          keep_going = false;
        } else if (ComputeServiceJobTypeNotSupportedMessage *msg = dynamic_cast<ComputeServiceJobTypeNotSupportedMessage *>(message.get())) {

          // update job state and remove from list
          if (msg->job->getType() == WorkflowJob::STANDARD) {
            StandardJob *job = (StandardJob *) msg->job;
            job->state = StandardJob::State::NOT_SUBMITTED;
            this->pending_standard_jobs.erase(job);

            // update the task states
            for (auto t: job->getTasks()) {
              t->setState(WorkflowTask::State::READY);
            }
          }

          if (msg->job->getType() == WorkflowJob::STANDARD) {
            PilotJob *job = (PilotJob *) msg->job;
            job->state = PilotJob::State::NOT_SUBMITTED;
            this->pending_pilot_jobs.erase(job);
          }

          // Forward the notification along the notification chain
          try {
            S4U_Mailbox::dputMessage(msg->job->popCallbackMailbox(),
                                     new ComputeServiceJobTypeNotSupportedMessage(msg->job, msg->compute_service, 0));
          } catch (std::shared_ptr<NetworkError> cause) {
            keep_going = true;
          }

        } else if (ComputeServiceStandardJobDoneMessage *msg = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
          // update job state
          StandardJob *job = msg->job;
          job->state = StandardJob::State::COMPLETED;

          // move the job from the "pending" list to the "completed" list
          this->pending_standard_jobs.erase(job);
          this->completed_standard_jobs.insert(job);

          // Forward the notification along the notification chain
          try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServiceStandardJobDoneMessage(job, msg->compute_service, 0.0));
          } catch (std::shared_ptr<NetworkError> cause) {
            keep_going = true;
          }
        } else if (ComputeServiceStandardJobFailedMessage *msg = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {

          // update job state
          StandardJob *job = msg->job;
          job->state = StandardJob::State::FAILED;

          // update the task states
          for (auto t: job->getTasks()) {
            t->incrementFailureCount();
            t->setState(WorkflowTask::State::READY);
          }

          // remove the job from the "pending" list
          this->pending_standard_jobs.erase(job);
          // put it in the "failed" list
          this->failed_standard_jobs.insert(job);

          // Forward the notification along the notification chain
          try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServiceStandardJobFailedMessage(job, msg->compute_service, std::move(msg->cause),
                                                                                0.0));
          } catch (std::shared_ptr<NetworkError> cause) {
            keep_going = true;
          }

        } else if (ComputeServicePilotJobStartedMessage *msg = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {

          // update job state
          PilotJob *job = msg->job;
          job->state = PilotJob::State::RUNNING;

          // move the job from the "pending" list to the "running" list
          this->pending_pilot_jobs.erase(job);
          this->running_pilot_jobs.insert(job);

          // Forward the notification to the source
          WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
          try {
            S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                     new ComputeServicePilotJobStartedMessage(job, msg->compute_service, 0.0));
          } catch (std::shared_ptr<NetworkError> cause) {
            keep_going = true;
          }

        } else if (ComputeServicePilotJobExpiredMessage *msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {

          // update job state
          PilotJob *job = msg->job;
          job->state = PilotJob::State::EXPIRED;

          // Remove the job from the "running" list
          this->running_pilot_jobs.erase(job);

          // Forward the notification to the source
          WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
          try {
            S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                     new ComputeServicePilotJobExpiredMessage(job, msg->compute_service, 0.0));
          } catch (std::shared_ptr<NetworkError> cause) {
            keep_going = true;
          }

        } else {
          throw std::runtime_error("JobManager::main(): Unexpected [" + message->getName() + "] message");
        }
      }

      WRENCH_INFO("Job Manager terminating");
      return 0;
    }

};
