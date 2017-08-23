/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "StandardJobExecutor.h"

#include "simulation/Simulation.h"
#include "logging/TerminalOutput.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simulation/SimulationMessage.h"
#include "services/storage_services/StorageService.h"
#include "services/ServiceMessage.h"
#include "services/compute_services/ComputeServiceMessage.h"
#include "workflow_job/StandardJob.h"
#include "exceptions/WorkflowExecutionException.h"
#include "workflow_job/PilotJob.h"
#include "StandardJobExecutorMessage.h"
#include "WorkunitMulticoreExecutor.h"
#include "Workunit.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(standard_job_executor, "Log category for Standard Job Executor");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param simulation: the simulation
     * @param hostname: the hostname of the host that should run this executor (could be the first compute resources - see below)
     * @param job: the job to execute
     * @param compute_resources: a list of <hostname, num_cores> tuples, which represent
     *           the compute resources the job should execute on
     * @param default_storage_service: a storage service (or nullptr)
     * @param plist: a property list
     *
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    StandardJobExecutor::StandardJobExecutor(Simulation *simulation,
                                             std::string callback_mailbox,
                                             std::string hostname,
                                             StandardJob *job,
                                             std::set<std::tuple<std::string, unsigned long>> compute_resources,
                                             StorageService *default_storage_service,
                                             std::map<std::string, std::string> plist) :
            S4U_DaemonWithMailbox("standard_job_executor", "standard_job_executor") {

      if ((job == nullptr) || (compute_resources.size() <= 0)) {
        throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): invalid arguments");
      }

      // Check that there is at least one core per host
      for (auto host : compute_resources) {
        if (std::get<1>(host) <= 0) {
          throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): there should be at least one core per host");
        }
      }

      // Check that there are enough cores to run the computational tasks
      int min_required_num_cores = 0;
      for (auto task : job->tasks) {
        min_required_num_cores = (min_required_num_cores < task->getMinNumCores() ? task->getMinNumCores() : min_required_num_cores);
      }

      bool enough_cores = false;
      for (auto host : compute_resources) {
        if (std::get<1>(host) >= min_required_num_cores) {
          enough_cores = true;
          break;
        }
      }

      if (!enough_cores) {
        throw std::runtime_error("StandardJobExecutor::StandardJobExecutor(): insufficient resources to run jobs");
      }

      // Set instance variables
      this->simulation = simulation;
      this->callback_mailbox = callback_mailbox;
      this->job = job;
      this->compute_resources = compute_resources;
      this->default_storage_service = default_storage_service;

      // Set properties
      // Set default properties
      for (auto p : this->default_property_values) {
        this->setProperty(p.first, p.second);
      }

      // Set specified properties
      for (auto p : plist) {
        this->setProperty(p.first, p.second);
      }

      // Compute the total number of cores and set initial core availabilities
      this->total_num_cores = 0;
      for (auto host : compute_resources) {
        this->total_num_cores += std::get<1>(host);
        this->core_availabilities[std::get<0>(host)] = std::get<1>(host);
      }

      // Start the daemon
      try {
        this->start(hostname);
      } catch (std::invalid_argument &e) {
        throw &e;
      }
    }


    /**
     * @brief Kill the executor
     */
    void StandardJobExecutor::kill() {
      // Kill all workunit executors
      for (auto workunit_executor :  this->workunit_executors)  {
        workunit_executor->kill();
      }
      this->kill_actor();
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int StandardJobExecutor::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_RED);
      WRENCH_INFO("New StandardJobExecutor starting (%s) with %d cores over %ld hosts: ",
                  this->mailbox_name.c_str(), this->total_num_cores, this->compute_resources.size());
      for (auto h : this->compute_resources) {
        WRENCH_INFO("  %s: %ld cores", std::get<0>(h).c_str(), std::get<1>(h));
      }

      /** Create all Workunits **/
      createWorkunits();

      /** Main loop **/
      while (true) {

        /** Dispatch currently pending work until no longer possible **/
        while (this->dispatchNextPendingWork());

        /** Process workunit completions **/
        processNextMessage();

        /** Detect Termination **/
        if (non_ready_workunits.size() + ready_workunits.size() +  running_workunits.size() == 0) {
          break;
        }

      }

      WRENCH_INFO("Standard Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     * @brief Dispatch one ready work unit, if any, to a new work unit executor, if possible
     * @return true if work was dispatched, false otherwise
     */
    bool StandardJobExecutor::dispatchNextPendingWork() {

      // If there is no ready work unit, there is nothing to dispatch
      if (this->ready_workunits.size() == 0) {
        return false;
      }

      // Get the first work out of the pending work queue
      std::shared_ptr<Workunit> ready_workunit = this->ready_workunits.front();

      // Compute its min and max cores requirements
      unsigned long min_num_cores = 0, max_num_cores = 0;

      if (ready_workunit->tasks.size() == 0) {
        min_num_cores = 1;
        max_num_cores = 1;
      } else if (ready_workunit->tasks.size() == 1) {
        min_num_cores = (unsigned long) ready_workunit->tasks[0]->getMinNumCores();
        max_num_cores = (unsigned long) ready_workunit->tasks[0]->getMaxNumCores();
      } else {
        throw std::runtime_error("StandardJobExecutor::dispatchNextPendingWork(): Found a work unit with 2 or more computational tasks");
      }


      // Find a host that can run it, picking the host with the largest number of cores
      // TODO: This is heuristic here, in which we try to maximumize the number of cores
      // TODO: per task. This many not be judicious since it may be better to run a bunch
      // TODO: of 1-core tasks in terms of parallel efficiency. The "how do I pick cores?"
      // TODO: method here is a feature of the executor, and perhaps could be configured
      // TODO: via some property, etc.
      std::string target_host = "";
      unsigned long target_num_cores = 0;

      for (auto h : this->compute_resources) {
        std::string hostname = std::get<0>(h);
        unsigned long num_available_cores = this->core_availabilities[hostname];
        if ((num_available_cores >= min_num_cores) && (num_available_cores > target_num_cores)) {
          target_host = hostname;
          target_num_cores = MIN(num_available_cores, max_num_cores);
        }
      }

      if (target_host == "") { // didn't find a suitable host
        return false;
      }


      // Create a workunit executor!
      WRENCH_INFO("Starting a worker unit executor with %ld cores on host %s", target_num_cores, target_host.c_str());

      WorkunitMulticoreExecutor *workunit_executor =
              new WorkunitMulticoreExecutor(this->simulation,
                                            target_host,
                                            target_num_cores,
                                            this->mailbox_name,
                                            ready_workunit,
                                            this->default_storage_service,
                                            this->getPropertyValueAsDouble(
                                                    StandardJobExecutorProperty::WORKUNIT_EXECUTOR_STARTUP_OVERHEAD));

      // Update core availabililties
      this->core_availabilities[target_host] -= target_num_cores;

      // Update data structures
      this->workunit_executors.insert(workunit_executor);
      this->ready_workunits.pop_front();
      this->running_workunits.insert(ready_workunit);

      return true;

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
    bool StandardJobExecutor::processNextMessage() {

      // Wait for a message
      std::unique_ptr<SimulationMessage> message;

      try {
        message = S4U_Mailbox::getMessage(this->mailbox_name);
      } catch (std::shared_ptr<NetworkError> cause) {
        // TODO: Send an exception above, and then send some "I failed" message to the service that created me
        return true;
      }

      if (message == nullptr) {
        WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
        return false;
      }

      WRENCH_INFO("Got a [%s] message", message->getName().c_str());

      if (WorkunitExecutorDoneMessage *msg = dynamic_cast<WorkunitExecutorDoneMessage *>(message.get())) {
        processWorkunitExecutorCompletion(msg->workunit_executor, msg->workunit);
        return true;

      } else if (WorkunitExecutorFailedMessage *msg = dynamic_cast<WorkunitExecutorFailedMessage *>(message.get())) {
        processWorkunitExecutorFailure(msg->workunit_executor, msg->workunit, msg->cause);
        return true;

      } else {
        throw std::runtime_error("Unexpected [" + message->getName() + "] message");
      }
    }



/**
 * @brief Process a workunit completion
 * @param workunit_executor: the workunit executor that completed the work unit
 * @param workunit: the workunit
 *
 * @throw std::runtime_error
 */
    void StandardJobExecutor::processWorkunitExecutorCompletion(
            WorkunitMulticoreExecutor *workunit_executor,
            std::shared_ptr<Workunit> workunit) {

      // Remove the workunit executor from the workunit executor list
      for (auto e : this->workunit_executors) {
        if (e == workunit_executor) {
          this->workunit_executors.erase(e);
          break;
        }
      }

      // Remove the work from the running work queue
      if (this->running_workunits.find(workunit) == this->running_workunits.end()) {
        throw std::runtime_error(
                "StandardJobExecutor::processWorkunitExecutorCompletion(): couldn't find a recently completed workunit in the running workunit list");
      }
      this->running_workunits.erase(workunit);

      // Add the workunit to the completed workunit list
      this->completed_workunits.insert(workunit);

      // Process task completions, if any
      for (auto task : workunit->tasks) {
        WRENCH_INFO("A workunit executor completed task %s (and its state is: %s)", task->getId().c_str(),
                    WorkflowTask::stateToString(task->getState()).c_str());

        // Increase the "completed tasks" count of the job
        this->job->incrementNumCompletedTasks();
      }

      // Send the callback to the originator if the job has completed (i.e., if this
      // work unit has no children)
      if (workunit->children.size() == 0) {

        // Erase all completed works for the job
        this->completed_workunits.clear();

        WRENCH_INFO("SENDING A CALLBACK to mailbox '%s'", this->callback_mailbox.c_str());
        try {
          S4U_Mailbox::dputMessage(this->callback_mailbox,
                                   new StandardJobExecutorDoneMessage(this->job, this,
                                                                            this->getPropertyValueAsDouble(
                                                                                    StandardJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
          WRENCH_INFO("Failed to send the callback... oh well");
          return;
        }
        WRENCH_INFO("CALLBACK SENT");
      } else {
        // Otherwise, update children
        for (auto child : workunit->children) {
          child->num_pending_parents--;
          if (child->num_pending_parents == 0) {
            // Make the child ready!
            if (this->non_ready_workunits.find(child) == this->non_ready_workunits.end()) {
              throw std::runtime_error(
                      "MulticoreComputeService::processWorkCompletion(): can't find non-ready child in non-ready set!");
            }
            this->non_ready_workunits.erase(child);
            this->ready_workunits.push_back(child);
          }
        }
      }

      return;
    }


/**
 * @brief Process a work failure
 * @param worker_thread: the worker thread that did the work
 * @param workunit: the workunit
 * @param cause: the cause of the failure
 */

    void StandardJobExecutor::processWorkunitExecutorFailure(
                                        WorkunitMulticoreExecutor *workunit_executor,
                                        std::shared_ptr<Workunit> workunit,
                                        std::shared_ptr<FailureCause> cause) {


      WRENCH_INFO("A workunit executor has failed to complete a workunit on behalf of job '%s'", this->job->getName().c_str());

      // Remove the workunit executor from the workunit executor list
      for (auto wt : this->workunit_executors) {
        if (wt == workunit_executor) {
          this->workunit_executors.erase(wt);
          break;
        }
      }

      // Remove the work from the running work queue
      if (this->running_workunits.find(workunit) == this->running_workunits.end()) {
        throw std::runtime_error(
                "StandardJobExecutor::processWorkunitExecutorFailure(): couldn't find a recently failed workunit in the running workunit list");
      }
      this->running_workunits.erase(workunit);

      // Remove all other workunits for the job in the "not ready" state
      this->non_ready_workunits.clear();

      // Remove all other workunits for the job in the "ready" state
      this->ready_workunits.clear();

      // Deal with running workunits!
      for (auto w : this->running_workunits) {
          if ((w->post_file_copies.size() != 0) || (w->pre_file_copies.size() != 0)) {
            throw std::runtime_error(
                    "StandardJobExecutor::processWorkunitExecutorFailure(): trying to cancel a running workunit that's doing some file copy operations - not supported (for now)");
          }
          // find the workunit executor  that's doing the work (lame iteration)
          for (auto wt :  this->workunit_executors) {
            if (wt->workunit == w) {
              wt->kill();
              break;
            }
          }
      }
      this->running_workunits.clear();

      // Deal with completed workunits
      this->completed_workunits.clear();

      // Send the notification back
      try {
        S4U_Mailbox::dputMessage(this->callback_mailbox,
                                 new StandardJobExecutorFailedMessage(this->job, this, cause,
                                                                          this->getPropertyValueAsDouble(
                                                                                  StandardJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> cause) {
        return;
      }

    }





/**
 * @brief Create all work for a newly dispatched job
 * @param job: the job
 */
    void StandardJobExecutor::createWorkunits() {

      std::shared_ptr<Workunit> pre_file_copies_work_unit = nullptr;
      std::vector<std::shared_ptr<Workunit>> task_work_units;
      std::shared_ptr<Workunit> post_file_copies_work_unit = nullptr;
      std::shared_ptr<Workunit> cleanup_workunit = nullptr;

      // Create the clean work unit, if any
      if (job->cleanup_file_deletions.size() > 0) {
        cleanup_workunit = std::shared_ptr<Workunit>(new Workunit({}, {}, {}, {}, job->cleanup_file_deletions));
      }

      // Create the pre_file_copies work unit, if any
      if (job->pre_file_copies.size() > 0) {
        pre_file_copies_work_unit = std::shared_ptr<Workunit>(new Workunit(job->pre_file_copies, {}, {}, {}, {}));
      }

      // Create the post_file_copies work unit, if any
      if (job->post_file_copies.size() > 0) {
        post_file_copies_work_unit = std::shared_ptr<Workunit>(new Workunit({}, {}, {}, job->post_file_copies, {}));
      }

      // Create the task work units, if any
      for (auto task : job->tasks) {
        task_work_units.push_back(std::shared_ptr<Workunit>(new Workunit({}, {task}, job->file_locations, {}, {})));
      }

      // Add dependencies from pre copies to possible successors
      if (pre_file_copies_work_unit != nullptr) {
        if (task_work_units.size() > 0) {
          for (auto twu: task_work_units) {
            Workunit::addDependency(pre_file_copies_work_unit, twu);
          }
        } else if (post_file_copies_work_unit != nullptr) {
          Workunit::addDependency(pre_file_copies_work_unit, post_file_copies_work_unit);
        } else if (cleanup_workunit != nullptr) {
          Workunit::addDependency(pre_file_copies_work_unit, cleanup_workunit);
        }
      }

      // Add dependencies from tasks to possible successors
      for (auto twu: task_work_units) {
        if (post_file_copies_work_unit != nullptr) {
          Workunit::addDependency(twu, post_file_copies_work_unit);
        } else if (cleanup_workunit != nullptr) {
          Workunit::addDependency(twu, cleanup_workunit);
        }
      }

      // Add dependencies from post copies to possible successors
      if (post_file_copies_work_unit != nullptr) {
        if (cleanup_workunit != nullptr) {
          Workunit::addDependency(post_file_copies_work_unit, cleanup_workunit);
        }
      }

      // Create a list of all work units
      std::vector<std::shared_ptr<Workunit>> all_work_units;
      if (pre_file_copies_work_unit) all_work_units.push_back(pre_file_copies_work_unit);
      for (auto twu : task_work_units) {
        all_work_units.push_back(twu);
      }
      if (post_file_copies_work_unit) all_work_units.push_back(post_file_copies_work_unit);
      if (cleanup_workunit) all_work_units.push_back(cleanup_workunit);

      // Insert work units in the ready or non-ready queues
      for (auto wu : all_work_units) {
        if (wu->num_pending_parents == 0) {
          this->ready_workunits.push_back(wu);
        } else {
          this->non_ready_workunits.insert(wu);
        }
      }

      return;
    }











    /**
     * @brief Set a property of the executor
     * @param property: the property
     * @param value: the property value
     */
    void StandardJobExecutor::setProperty(std::string property, std::string value) {
      this->property_list[property] = value;
    }

    /**
     * @brief Get a property of the executor a string
     * @param property: the property
     * @return the property value as a string
     *
     * @throw std::runtime_error
     */
    std::string StandardJobExecutor::getPropertyValueAsString(std::string property) {
      if (this->property_list.find(property) == this->property_list.end()) {
        throw std::runtime_error("Service::getPropertyValueAsString(): Cannot find value for property " + property +
                                 " (perhaps a derived service class does not provide a default value?)");
      }
      return this->property_list[property];
    }

    /**
     * @brief Get a property of the executor as a double
     * @param property: the property
     * @return the property value as a double
     *
     * @throw std::runtime_error
     */
    double StandardJobExecutor::getPropertyValueAsDouble(std::string property) {
      double value;
      if (sscanf(this->getPropertyValueAsString(property).c_str(), "%lf", &value) != 1) {
        throw std::runtime_error("Service::getPropertyValueAsDouble(): Invalid double property value " + property + " " +
                                 this->getPropertyValueAsString(property));
      }
      return value;
    }

};

