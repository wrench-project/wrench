/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <wrench/wms/WMS.h>

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/JobManager.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/wms/WMS.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(job_manager, "Log category for Job Manager");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param wms: the wms for which this manager is working
     */
    JobManager::JobManager(WMS *wms) :
            Service(wms->hostname, "job_manager", "job_manager") {

      this->wms = wms;

      // Get myself known to schedulers
      if (this->wms->standard_job_scheduler) {
        this->wms->standard_job_scheduler->setJobManager(this);
      }
      if (this->wms->pilot_job_scheduler) {
        this->wms->pilot_job_scheduler->setJobManager(this);
      }
    }

    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    JobManager::~JobManager() {
      this->jobs.clear();
    }

    /**
     * @brief Kill the job manager (brutally terminate the daemon, clears all jobs)
     */
    void JobManager::kill() {
      this->killActor();
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
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the standard job)
     * @param file_locations: a map that specifies on which storage services input/output files should be read/written.
     *         When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     * @param pre_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         before task executions begin. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param post_file_copies: a set of tuples that specify which file copy operations should be completed
     *                         after task executions end. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param cleanup_file_deletions: a set of file tuples that specify file deletion operations that should be completed
     *                                at the end of the job. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks,
                                               std::map<WorkflowFile *, StorageService *> file_locations,
                                               std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                                               std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                                               std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions) {

      // Do a sanity check of everything (looking for nullptr)
      for (auto t : tasks) {
        if (t == nullptr) {
          throw std::invalid_argument("JobManager::createStandardJob(): nullptr task in the task vector");
        }
      }

      for (auto fl : file_locations) {
        if (fl.first == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr workflow file in the file_locations map");
        }
        if (fl.second == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr storage service in the file_locations map");
        }
      }

      for (auto fc : pre_file_copies) {
        if (std::get<0>(fc) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr workflow file in the pre_file_copies set");
        }
        if (std::get<1>(fc) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr src storage service in the pre_file_copies set");
        }
        if (std::get<2>(fc) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr dst storage service in the pre_file_copies set");
        }
      }

      for (auto fc : post_file_copies) {
        if (std::get<0>(fc) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr workflow file in the post_file_copies set");
        }
        if (std::get<1>(fc) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr src storage service in the post_file_copies set");
        }
        if (std::get<2>(fc) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr dst storage service in the post_file_copies set");
        }
      }

      for (auto fd : cleanup_file_deletions) {
        if (std::get<0>(fd) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr workflow file in the cleanup_file_deletions set");
        }
        if (std::get<1>(fd) == nullptr) {
          throw std::invalid_argument(
                  "JobManager::createStandardJob(): nullptr storage service in the cleanup_file_deletions set");
        }
      }

      StandardJob *raw_ptr = new StandardJob(tasks, file_locations, pre_file_copies, post_file_copies,
                                             cleanup_file_deletions);
      std::unique_ptr<WorkflowJob> job = std::unique_ptr<StandardJob>(raw_ptr);

      this->jobs.insert(std::make_pair(raw_ptr, std::move(job)));
      return raw_ptr;
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks  (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the list)
     * @param file_locations: a map that specifies on which storage services input/output files should be read/written.
     *                        When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks,
                                               std::map<WorkflowFile *, StorageService *> file_locations) {
      if (tasks.empty()) {
        throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
      }

      return this->createStandardJob(tasks, file_locations, {}, {}, {});
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a task (which must be ready)
     * @param file_locations: a map that specifies on which storage services input/output files should be read/written.
     *                When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
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
     * @param num_hosts: the number of hosts required by the pilot job
     * @param num_cores_per_host: the number of cores per host required by the pilot job
     * @param ram_per_host: the number of bytes of RAM required by the pilot job on each host
     * @param duration: the pilot job's duration in seconds
     * @return the pilot job
     *
     * @throw std::invalid_argument
     */
    PilotJob *JobManager::createPilotJob(unsigned long num_hosts,
                                         unsigned long num_cores_per_host,
                                         double ram_per_host,
                                         double duration) {
      if ((ram_per_host < 0) || (duration <= 0.0)) {
        throw std::invalid_argument("JobManager::createPilotJob(): Invalid arguments");
      }
      PilotJob *raw_ptr = new PilotJob(this->wms->workflow, num_hosts, num_cores_per_host, ram_per_host, duration);
      std::unique_ptr<WorkflowJob> job = std::unique_ptr<PilotJob>(raw_ptr);
      this->jobs[raw_ptr] = std::move(job);
      return raw_ptr;
    }

//    /**
//     * @brief Submit a job to a compute service
//     *
//     * @param job: a workflow job
//     * @param compute_service: a compute service
//     *
//     * @throw std::invalid_argument
//     * @throw WorkflowExecutionException
//     */
//    void JobManager::submitJob(WorkflowJob *job, ComputeService *compute_service) {
//
//      if ((job == nullptr) || (compute_service == nullptr)) {
//        throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
//      }
//
//      // Push back the mailbox_name of the manager,
//      // so that it will getMessage the initial callback
//      job->pushCallbackMailbox(this->mailbox_name);
//
//      // Update the job state and insert it into the pending list
//      switch (job->getType()) {
//        case WorkflowJob::STANDARD: {
//          ((StandardJob *) job)->state = StandardJob::PENDING;
//          for (auto t : ((StandardJob *) job)->tasks) {
//            t->setState(WorkflowTask::State::PENDING);
//          }
//          this->pending_standard_jobs.insert((StandardJob *) job);
//          break;
//        }
//        case WorkflowJob::PILOT: {
//          ((PilotJob *) job)->state = PilotJob::PENDING;
//          this->pending_pilot_jobs.insert((PilotJob *) job);
//          break;
//        }
//      }
//
//      // Submit the job to the service
//      try {
//        compute_service->submitJob(job);
//        job->setParentComputeService(compute_service);
//      } catch (WorkflowExecutionException &e) {
//        throw;
//      }
//
//    }

    /**
     * @brief Submit a job to compute service
     *
     * @param job: a workflow job
     * @param compute_service: a compute service
     * @param service_specific_args: arguments specific for compute services:
     *      - to a MultihostMulticoreComputeService: {}
     *      - to a BatchService: {"-t":"<int>" (requested number of minutes),"-N":"<int>" (number of requested hosts),"-c":"<int>" (number of requestes cores per host)}
     *      - to a VirtualizedClusterService: {}
     *      - to a CloudService: {}
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void JobManager::submitJob(WorkflowJob *job, ComputeService *compute_service,
                               std::map<std::string, std::string> service_specific_args) {

      if ((job == nullptr) || (compute_service == nullptr)) {
        throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
      }

      // Push back the mailbox_name of the manager,
      // so that it will getMessage the initial callback
      job->pushCallbackMailbox(this->mailbox_name);

      // Update the job state and insert it into the pending list
      switch (job->getType()) {
        case WorkflowJob::STANDARD: {
          // Do a sanity check
          for (auto t : ((StandardJob *) job)->tasks) {
            if ((t->getState() == WorkflowTask::State::COMPLETED) or
                (t->getState() == WorkflowTask::State::PENDING)) {
              throw std::invalid_argument("JobManager()::submitJob(): task " + t->getID() +
                                          " cannot be submitted as part of a standard job because its state is " +
                                          WorkflowTask::stateToString(t->getState()));
            }
          }
          // Modify task states
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
        compute_service->submitJob(job, service_specific_args);
        job->setParentComputeService(compute_service);
      } catch (WorkflowExecutionException &e) {
        throw;
      }

    }

    /**
     * @brief Terminate a job (standard or pilot) that hasn't completed/expired/failed yet
     * @param job: the job to be terminated
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
        throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)));
      }

      try {
        job->getParentComputeService()->terminateJob(job);
      } catch (std::exception &e) {
        throw;
      }

      if (job->getType() == WorkflowJob::STANDARD) {
        ((StandardJob *) job)->state = StandardJob::State::TERMINATED;
        for (auto task : ((StandardJob *) job)->tasks) {
          switch (task->getInternalState()) {
            case WorkflowTask::TASK_NOT_READY:
              task->setState(WorkflowTask::State::NOT_READY);
              break;
            case WorkflowTask::TASK_READY:
              task->setState(WorkflowTask::State::READY);
              break;
            case WorkflowTask::TASK_COMPLETED:
              task->setState(WorkflowTask::State::COMPLETED);
              break;
            case WorkflowTask::TASK_RUNNING:
            case WorkflowTask::TASK_FAILED:
              task->setState(WorkflowTask::State::NOT_READY);
              break;
          }
        }
        // Make second pass to fix NOT_READY states
        for (auto task : ((StandardJob *) job)->tasks) {
          if (task->getState() == WorkflowTask::State::NOT_READY) {
            bool ready = true;
            for (auto parent : task->getWorkflow()->getTaskParents(task)) {
              if (parent->getState() != WorkflowTask::State::COMPLETED) {
                ready = false;
              }
            }
            if (ready) {
              task->setState(WorkflowTask::State::READY);
            }
          }
        }
      } else if (job->getType() == WorkflowJob::PILOT) {
        ((PilotJob *) job)->state = PilotJob::State::TERMINATED;
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
     * @brief Forget a job (to free memory, only once a job has completed or failed)
     *
     * @param job: a job to forget
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
          throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new JobCannotBeForgotten(job)));
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
        // At this point, it's a job that was never submitted!
        if (this->jobs.find(job) != this->jobs.end()) {
          this->jobs.erase(job);
          return;
        }
        throw std::invalid_argument("JobManager::forgetJob(): unknown standard job");
      }

      if (job->getType() == WorkflowJob::PILOT) {
        if ((this->pending_pilot_jobs.find((PilotJob *) job) != this->pending_pilot_jobs.end()) ||
            (this->running_pilot_jobs.find((PilotJob *) job) != this->running_pilot_jobs.end())) {
          throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new JobCannotBeForgotten(job)));
        }
        if (this->completed_pilot_jobs.find((PilotJob *) job) != this->completed_pilot_jobs.end()) {
          this->jobs.erase(job);
          return;
        }
        // At this point, it's a job that was never submitted!
        if (this->jobs.find(job) != this->jobs.end()) {
          this->jobs.erase(job);
          return;
        }
        throw std::invalid_argument("JobManager::forgetJob(): unknown pilot job");
      }

    }

    /**
     * @brief Main method of the daemon that implements the JobManager
     * @return 0 on success
     */
    int JobManager::main() {

      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

      WRENCH_INFO("New Job Manager starting (%s)", this->mailbox_name.c_str());

      bool keep_going = true;
      while (keep_going) {
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
          WRENCH_INFO("Waiting for a message");
          message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
          continue;
        }

        if (message == nullptr) {
          WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
          break;
        }
        // Clear finished asynchronous dputMessage()
//        S4U_Mailbox::clear_dputs();

        WRENCH_INFO("Job Manager got a %s message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
          // There shouldn't be any need to clean any state up
          keep_going = false;

        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
          // update job state
          StandardJob *job = msg->job;
          job->state = StandardJob::State::COMPLETED;

          // Update all visible states
          for (auto task : job->tasks) {
            if (task->getInternalState() == WorkflowTask::InternalState::TASK_COMPLETED) {
              task->setState(WorkflowTask::State::COMPLETED);
            } else {
              throw std::runtime_error("JobManager::main(): got a 'job done' message, but task " +
                                       task->getID() + " does not have a TASK_COMPLETED internal state (" +
                                       WorkflowTask::stateToString(task->getInternalState()) + ")");
            }
            auto children = this->wms->getWorkflow()->getTaskChildren(task);
            for (auto child : children) {
              switch (child->getInternalState()) {
                case WorkflowTask::InternalState::TASK_NOT_READY:
                case WorkflowTask::InternalState::TASK_RUNNING:
                case WorkflowTask::InternalState::TASK_FAILED:
                case WorkflowTask::InternalState::TASK_COMPLETED:
                  // no nothing
                  break;
                case WorkflowTask::InternalState::TASK_READY:
                  if (child->getState() == WorkflowTask::State::NOT_READY) {
                    bool all_parents_ready = true;
                    for (auto parent : child->getWorkflow()->getTaskParents(child)) {
                      if (parent->getState() != WorkflowTask::State::COMPLETED) {
                        all_parents_ready = false;
                        break;
                      }
                    }
                    if (all_parents_ready) {
                      child->setState(WorkflowTask::State::READY);
                    }
                  }
                  break;
              }
            }
          }

          // move the job from the "pending" list to the "completed" list
          this->pending_standard_jobs.erase(job);
          this->completed_standard_jobs.insert(job);

          // Forward the notification along the notification chain
          std::string callback_mailbox = job->popCallbackMailbox();
          if (not callback_mailbox.empty()) {
            try {
              S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                       new ComputeServiceStandardJobDoneMessage(job, msg->compute_service, 0.0));
            } catch (std::shared_ptr<NetworkError> &cause) {
              // ignore
            }
          }
          keep_going = true;

        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {

          // update job state
          StandardJob *job = msg->job;
          job->state = StandardJob::State::FAILED;

          // update the task states and failure counts!
          for (auto t: job->getTasks()) {
            if (t->getInternalState() == WorkflowTask::InternalState::TASK_COMPLETED) {
              t->setState(WorkflowTask::State::COMPLETED);
              for (auto child : this->wms->getWorkflow()->getTaskChildren(t)) {
                switch (child->getInternalState()) {
                  case WorkflowTask::InternalState::TASK_NOT_READY:
                  case WorkflowTask::InternalState::TASK_RUNNING:
                  case WorkflowTask::InternalState::TASK_FAILED:
                  case WorkflowTask::InternalState::TASK_COMPLETED:
                    // no nothing
                    break;
                  case WorkflowTask::InternalState::TASK_READY:
                    child->setState(WorkflowTask::State::READY);
                    break;
                }
              }

            } else if (t->getInternalState() == WorkflowTask::InternalState::TASK_READY) {
              t->setState(WorkflowTask::State::READY);
              t->incrementFailureCount();

            } else if (t->getInternalState() == WorkflowTask::InternalState::TASK_FAILED) {
              bool ready = true;
              for (auto parent : this->wms->getWorkflow()->getTaskParents(t)) {
                if (parent->getInternalState() != WorkflowTask::InternalState::TASK_COMPLETED) {
                  ready = false;
                }
              }
              if (ready) {
                t->setState(WorkflowTask::State::READY);
              } else {
                t->setState(WorkflowTask::State::NOT_READY);
              }
            }
          }

          // remove the job from the "pending" list
          this->pending_standard_jobs.erase(job);
          // put it in the "failed" list
          this->failed_standard_jobs.insert(job);

          // Forward the notification along the notification chain
          try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServiceStandardJobFailedMessage(job, msg->compute_service,
                                                                                std::move(msg->cause),
                                                                                0.0));
          } catch (std::shared_ptr<NetworkError> &cause) {
            keep_going = true;
          }

        } else if (auto msg = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {

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
          } catch (std::shared_ptr<NetworkError> &cause) {
            keep_going = true;
          }

        } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {

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
          } catch (std::shared_ptr<NetworkError> &cause) {
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
