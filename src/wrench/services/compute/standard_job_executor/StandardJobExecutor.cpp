/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cfloat>
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "wrench/services/compute/standard_job_executor/WorkunitMulticoreExecutor.h"
#include "wrench/services/compute/standard_job_executor/Workunit.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/workflow/job/StandardJob.h"

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/workflow/job/PilotJob.h"
#include "StandardJobExecutorMessage.h"

#include "wrench/util/PointerUtil.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(standard_job_executor, "Log category for Standard Job Executor");

namespace wrench {

    /**
     * @brief Destructor
     */
    StandardJobExecutor::~StandardJobExecutor() {
      this->default_property_values.clear();
    }

    /**
     * @brief Constructor
     *
     * @param simulation: a reference to a simulation object
     * @param callback_mailbox: the mailbox to which a reply will be sent
     * @param hostname: the hostname of the host that should run this executor (could be the first compute resources - see below)
     * @param job: the job to execute
     * @param compute_resources: a non-empty list of <hostname, num_cores, memory> tuples, which represent
     *           the compute resources the job should execute on
     *              - If num_cores == ComputeService::ALL_CORES, then ALL the cores of the host are used
     *              - If memory == ComputeService::ALL_RAM, then ALL the ram of the host is used
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
                                             std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                                             StorageService *default_storage_service,
                                             std::map<std::string, std::string> plist) :
            Service(hostname, "standard_job_executor", "standard_job_executor") {

      if ((job == nullptr) || (compute_resources.empty())) {
        throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): invalid arguments");
      }

      // Check that hosts exist!
      for (auto h : compute_resources) {
        if (not simulation->hostExists(std::get<0>(h))) {
          throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): Host '" + std::get<0>(h) + "' does not exit!");
        }
      }

      // Check that there is at least one core per host but not too many cores
      for (auto host : compute_resources) {
        if (std::get<1>(host) == 0) {
          throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): there should be at least one core per host");
        }
        if (std::get<1>(host) < ComputeService::ALL_CORES) {
          if (std::get<1>(host) > S4U_Simulation::getNumCores(std::get<0>(host))) {
            throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): host " + std::get<0>(host) +
                                        " has only " + std::to_string(S4U_Simulation::getNumCores(std::get<0>(host))) + " cores");
          }
        } else {
          // Set the num_cores to the maximum
          std::get<1>(host) = S4U_Simulation::getNumCores(std::get<0>(host));
        }
      }

      // Check that there is at least zero byte of memory per host, but not too many bytes
      for (auto host : compute_resources) {
        if (std::get<2>(host) < 0) {
          throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): the number of bytes per host should be non-negative");
        }
        if (std::get<2>(host) < ComputeService::ALL_RAM) {
          double host_memory_capacity = S4U_Simulation::getHostMemoryCapacity(std::get<0>(host));
          if (std::get<2>(host) > host_memory_capacity) {
            throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): host " + std::get<0>(host) +
                                        " has only " + std::to_string(
                    S4U_Simulation::getHostMemoryCapacity(std::get<0>(host))) + " bytes of RAM");
          }
        } else {
          // Set the memory to the maximum
          std::get<2>(host) = S4U_Simulation::getHostMemoryCapacity(std::get<0>(host));
        }
      }


      // Check that there are enough cores to run the computational tasks
      unsigned long max_min_required_num_cores = 0;
      for (auto task : job->tasks) {
        max_min_required_num_cores = (max_min_required_num_cores < task->getMinNumCores() ? task->getMinNumCores() : max_min_required_num_cores);
      }

      bool enough_cores = false;
      for (auto host : compute_resources) {
        unsigned long num_cores_on_hosts = std::get<1>(host);
        if (num_cores_on_hosts == ComputeService::ALL_CORES) {
          num_cores_on_hosts = S4U_Simulation::getNumCores(std::get<0>(host));
        }

        if (num_cores_on_hosts >= max_min_required_num_cores) {
          enough_cores = true;
          break;
        }
      }

      if (!enough_cores) {
        throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): insufficient core resources to run the job");
      }


      // Check that there is enough RAM to run the computational tasks
      double max_required_ram = 0.0;
      for (auto task : job->tasks) {
        max_required_ram = (max_required_ram < task->getMemoryRequirement() ? task->getMemoryRequirement() : max_required_ram);
      }


      bool enough_ram = false;
      for (auto host : compute_resources) {
        if (std::get<2>(host) >= max_required_ram) {
          enough_ram = true;
          break;
        }
      }


      if (!enough_ram) {
        throw std::invalid_argument("StandardJobExecutor::StandardJobExecutor(): insufficient memory resources to run the job");
      }

      // Set instance variables
      this->simulation = simulation;
      this->callback_mailbox = callback_mailbox;
      this->job = job;
      this->default_storage_service = default_storage_service;


      // set properties
      this->setProperties(this->default_property_values, plist);

      // Compute the total number of cores and set initial core availabilities
      this->total_num_cores = 0;
      for (auto host : compute_resources) {
        unsigned long num_cores = std::get<1>(host);
        if (num_cores == ComputeService::ALL_CORES) {
          num_cores = simulation->getHostNumCores(std::get<0>(host));
        }
        this->total_num_cores += num_cores;
        this->core_availabilities.insert(std::make_pair(std::get<0>(host), num_cores));
      }

      // Compute the total ram and set initial ram availabilities
      this->total_ram = 0.0;
      for (auto host : compute_resources) {
        double ram = std::get<2>(host);
        if (ram == ComputeService::ALL_RAM) {
          ram = simulation->getHostMemoryCapacity(std::get<0>(host));
        }
        this->total_ram += ram;
        this->ram_availabilities.insert(std::make_pair(std::get<0>(host),  ram));
      }

      // Create my compute resources record
      for (auto host : compute_resources) {
        unsigned long num_cores = std::get<1>(host);
        if (num_cores == ComputeService::ALL_CORES) {
          num_cores = simulation->getHostNumCores(std::get<0>(host));
        }
        double ram = std::get<2>(host);
        if (ram == ComputeService::ALL_RAM) {
          ram = simulation->getHostMemoryCapacity(std::get<0>(host));
        }
        this->compute_resources.insert(std::make_tuple(std::get<0>(host), num_cores, ram));
      }

    }

    /**
     * @brief Kill the executor
     */
    void StandardJobExecutor::kill() {

      this->acquireDaemonLock();

      // Kill all Workunit executors
      for (auto const &wue : this->running_workunit_executors) {
        wue->kill();
      }

      // Then kill the actor
      this->killActor();

      this->releaseDaemonLock();


    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int StandardJobExecutor::main() {

      TerminalOutput::setThisProcessLoggingColor(COLOR_RED);
      WRENCH_INFO("New StandardJobExecutor starting (%s) with %d cores and %.2lf bytes of RAM over %ld hosts: ",
                  this->mailbox_name.c_str(), this->total_num_cores, this->total_ram, this->core_availabilities.size());
      for (auto h : this->core_availabilities) {
        WRENCH_INFO("  %s: %ld cores", std::get<0>(h).c_str(), std::get<1>(h));
      }

      /** Create all Workunits **/
      createWorkunits();

      /** Main loop **/
      while (true) {

        /** Dispatch currently ready workunits, as much as possible  **/
        dispatchReadyWorkunits();

        /** Process workunit completions **/
        if (!processNextMessage()) {
          break;
        }

        /** Detect Termination **/
        if (this->non_ready_workunits.empty() and  this->ready_workunits.empty() and  this->running_workunits.empty()) {
//          std::cerr << "CANARY 1: " << (*(this->completed_workunits. begin())).use_count() << "\n";
//          std::shared_ptr<Workunit> canary = std::move(*(this->completed_workunits.begin()));
//          std::cerr << "CANARY 2: " << canary.use_count() << "\n";
//          this->completed_workunits.erase(*(this->completed_workunits. begin()));
//          this->completed_workunits.erase(canary);
//          std::cerr << "WTF: " << this->completed_workunits.size() << "\n";
//          std::cerr << "CANARY 3: " << canary.use_count() << "\n";
          this->completed_workunits.clear();

          break;
        }

      }

      WRENCH_INFO("Standard Job Executor on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     * @brief Computes the minimum number of cores required to execute a work unit
     * @param wu: the work unit
     * @return a number of cores
     *
     * @throw std::runtime_error
     */
    unsigned long StandardJobExecutor::computeWorkUnitMinNumCores(Workunit *wu) {
      unsigned long minimum_num_cores;
      if (wu->tasks.empty()) {
        minimum_num_cores = 1;
      } else if (wu->tasks.size() == 1) {
        minimum_num_cores = wu->tasks[0]->getMinNumCores();
      } else {
        throw std::runtime_error(
                "StandardJobExecutor::computeWorkUnitMinNumCores(): Found a workunit with more than one computational tasks!!");
      }
      return minimum_num_cores;
    }

    /**
     * @brief Computes the desired number of cores required to execute a work unit
     * @param wu: the work unit
     * @return a number of cores
     *
     * @throw std::runtime_error
     */
    unsigned long StandardJobExecutor::computeWorkUnitDesiredNumCores(Workunit *wu) {
      unsigned long desired_num_cores;
      if (wu->tasks.empty()) {
        desired_num_cores = 1;

      } else if (wu->tasks.size() == 1) {
        std::string core_allocation_algorithm =
                this->getPropertyValueAsString(StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM);

        if (core_allocation_algorithm == "maximum") {
          desired_num_cores = wu->tasks[0]->getMaxNumCores();
        } else if (core_allocation_algorithm == "minimum") {
          desired_num_cores = wu->tasks[0]->getMinNumCores();
        } else {
          throw std::runtime_error("StandardjobExecutor::computeWorkUnitDesiredNumCores(): Unknown StandardJobExecutorProperty::CORE_ALLOCATION_ALGORITHM property '"
                                   + core_allocation_algorithm + "'");
        }
      } else {
        throw std::runtime_error(
                "StandardjobExecutor::computeWorkUnitDesiredNumCores(): Found a workunit with more than one computational tasks!!");
      }
      return desired_num_cores;
    }

    /**
    * @brief Computes the desired amount of RAM required to execute a work unit
    * @param wu: the work unit
    * @return a number of bytes
    *
    * @throw std::runtime_error
    */
    double StandardJobExecutor::computeWorkUnitMinMemory(Workunit *wu) {
      double  min_ram;
      if (wu->tasks.empty()) {
        min_ram  = 0.0;

      } else if (wu->tasks.size() == 1) {
        min_ram = wu->tasks[0]->getMemoryRequirement();
      } else {
        throw std::runtime_error(
                "StandardjobExecutor::computeWorkUnitMinMemory(): Found a workunit with more than one computational tasks!!");
      }
      return min_ram;
    }

    /**
     * @brief Dispatch ready work units to hosts/cores, while possible
     */
    void StandardJobExecutor::dispatchReadyWorkunits() {

      // If there is no ready work unit, there is nothing to dispatch
      if (this->ready_workunits.empty()) {
        return;
      }

      // Don't kill me while I am doing this!
      this->acquireDaemonLock();

//      std::cerr << "** IN DISPATCH READY WORK UNITS\n";
//      for (auto wu : this->ready_workunits) {
//        std::cerr << "WU: num_comp_tasks " << wu->tasks.size() << "\n";
//        for (auto t : wu->tasks) {
//          std::cerr << "    - flops = " << t->getFlops() << ", min_cores = " << t->getMinNumCores() << ", max_cores = " << t->getMaxNumCores() << "\n";
//        }
//      }


      // Get an ordered (by the task selection algorithm) list of the ready workunits
      std::vector<Workunit *> sorted_ready_workunits = sortReadyWorkunits();

//      std::cerr << "** SORTED\n";
//      for (auto wu : sorted_ready_workunits) {
//        std::cerr << "WU: num_comp_tasks " << wu->tasks.size() << "\n";
//        for (auto t : wu->tasks) {
//          std::cerr << "    - flops = " << t->getFlops() << ", min_cores = " << t->getMinNumCores() << ", max_cores = " << t->getMaxNumCores() << "\n";
//        }
//      }

      // Go through the workunits in order of priority and dispatch each them to
      // hosts/cores, if possible
      for (auto wu : sorted_ready_workunits) {

        // Compute the workunit's minimum number os cores, desired number of cores, and minimum amount of ram
        unsigned long minimum_num_cores;
        unsigned long desired_num_cores;
        double required_ram;

        try {
          minimum_num_cores = computeWorkUnitMinNumCores(wu);
          desired_num_cores = computeWorkUnitDesiredNumCores(wu);
          required_ram = computeWorkUnitMinMemory(wu);
        } catch (std::runtime_error &e) {
          this->releaseDaemonLock();
          throw;
        }

        // Find a host on which to run the workunit, and on how many cores
        std::string target_host = "";
        unsigned long target_num_cores = 0;

        WRENCH_INFO("Looking for a host to run a work unit that needs at least %ld cores, and would like %ld cores, and requires %.2lf bytes of RAM",
                    minimum_num_cores, desired_num_cores, required_ram);
        std::string host_selection_algorithm =
                this->getPropertyValueAsString(StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM);

//        std::cerr << "** FINDING A HOST USING " << host_selection_algorithm << "\n";

        if (host_selection_algorithm == "best_fit") {
          unsigned long target_slack = 0;

          for (auto const &h : this->core_availabilities) {
            std::string const &hostname = std::get<0>(h);
//              WRENCH_INFO("Looking at host %s", hostname.c_str());

            // Does the host have enough cores?
            unsigned long num_available_cores = this->core_availabilities[hostname];
            if (num_available_cores < minimum_num_cores) {
//              WRENCH_INFO("Not enough cores!");
              continue;
            }

            // Does the host have enough RAM?
            double available_ram = this->ram_availabilities[hostname];
            if (available_ram < required_ram) {
              WRENCH_INFO("Not enough RAM!");
              continue;
            }

//            std::cerr << "    HOST COULD WORK \n";

            unsigned long tentative_target_num_cores = MIN(num_available_cores, desired_num_cores);
            unsigned long tentative_target_slack = num_available_cores - tentative_target_num_cores;

            if ((target_host == "") ||
                (tentative_target_num_cores > target_num_cores) ||
                ((tentative_target_num_cores == target_num_cores) && (target_slack > tentative_target_slack))) {
//              std::cerr << "YEAH!!!\n";
              target_host = hostname;
              target_num_cores = tentative_target_num_cores;
              target_slack = tentative_target_slack;
            }
          }
        } else {
          this->releaseDaemonLock();
          throw std::runtime_error("Unknown StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM property '"
                                   + host_selection_algorithm + "'");
        }


        if (target_host == "") { // didn't find a suitable host
          WRENCH_INFO("Didn't find a suitable host");
//          std::cerr << "DID NOT FIND A HOST, GOING TO NEXT WORK UNIT\n";
          continue;
        }

//        std::cerr << "FOUND A HOST!!\n";


        // Create a workunit executor!
        WRENCH_INFO("Starting a worker unit executor with %ld cores on host %s",
                    target_num_cores, target_host.c_str());

//        std::cerr << "CREATING A WORKUNIT EXECUTOR\n";

        std::shared_ptr<WorkunitMulticoreExecutor> workunit_executor = std::shared_ptr<WorkunitMulticoreExecutor>(
                new WorkunitMulticoreExecutor(this->simulation,
                                              target_host,
                                              target_num_cores,
                                              required_ram,
                                              this->mailbox_name,
                                              wu,
                                              this->default_storage_service,
                                              this->getPropertyValueAsDouble(
                                                      StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD)));

        workunit_executor->simulation = this->simulation;
        workunit_executor->start(workunit_executor, true);

        // Update core availabilities
        this->core_availabilities[target_host] -= target_num_cores;
        // Update RAM availabilities
        this->ram_availabilities[target_host] -= required_ram;


        // Update data structures
        this->running_workunit_executors.insert(workunit_executor);

        for (auto it = this->ready_workunits.begin();
             it != this->ready_workunits.end(); it++) {
          if ((*it).get() == wu) {
            PointerUtil::moveUniquePtrFromSetToSet(it, &(this->ready_workunits), &(this->running_workunits));
            break;
          }
        }

      }

      sorted_ready_workunits.clear();

      this->releaseDaemonLock();

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
      } catch (std::shared_ptr<NetworkError> &cause) {
        // TODO: Send an exception above, and then send some "I failed" message to the service that created me?
        return true;
      } catch (std::shared_ptr<FatalFailure> &cause) {
        WRENCH_INFO("Got a Unknown Failure during a communication... likely this means we're all done. Aborting");
        return false;
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
            Workunit *workunit) {

      // Don't kill me while I am doing this
      this->acquireDaemonLock();

      // Update core availabilities
      this->core_availabilities[workunit_executor->getHostname()] += workunit_executor->getNumCores();
      // Update RAM availabilities
      this->ram_availabilities[workunit_executor->getHostname()] += workunit_executor->getMemoryUtilization();

      // Remove the workunit executor from the workunit executor list
      for (auto it = this->running_workunit_executors.begin(); it != this->running_workunit_executors.end(); it++) {
        if ((*it).get() == workunit_executor) {
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_workunit_executors), &(this->finished_workunit_executors));
          break;
        }
      }

      // Find the workunit in the running workunit queue
      bool found_it = false;
      for (auto it = this->running_workunits.begin(); it != this->running_workunits.end(); it++) {
        if ((*it).get() == workunit) {
          PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_workunits), &(this->completed_workunits));
          found_it = true;
          break;
        }
      }
      if (!found_it) {
        throw std::runtime_error(
                "StandardJobExecutor::processWorkunitExecutorCompletion(): couldn't find a recently completed workunit in the running workunit list");
      }

      // Process task completions, if any
      for (auto task : workunit->tasks) {
        WRENCH_INFO("A workunit executor completed task %s (and its state is: %s)", task->getId().c_str(),
                    WorkflowTask::stateToString(task->getState()).c_str());

        // Increase the "completed tasks" count of the job
        this->job->incrementNumCompletedTasks();
      }


      // Send the callback to the originator if the job has completed
      if ((this->non_ready_workunits.empty()) &&
          (this->ready_workunits.empty()) &&
          (this->running_workunits.empty())) {

        // Erase all completed works for the job
        this->completed_workunits.clear();

        try {
          S4U_Mailbox::putMessage(this->callback_mailbox,
                                  new StandardJobExecutorDoneMessage(this->job, this,
                                                                     this->getPropertyValueAsDouble(
                                                                             StandardJobExecutorProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          WRENCH_INFO("Failed to send the callback... oh well");
          this->releaseDaemonLock();
          return;
        }
      } else {
        // Otherwise, update children
        for (auto child : workunit->children) {
          child->num_pending_parents--;
          if (child->num_pending_parents == 0) {
            // Make the child ready!

            // Find the workunit in the running workunig queue
            bool found_it = false;
            for (auto it = this->non_ready_workunits.begin(); it != this->non_ready_workunits.end(); it++) {
              if ((*it).get() == child) {
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->non_ready_workunits), &(this->ready_workunits));
                found_it = true;
                break;
              }
            }
            if (!found_it) {
              throw std::runtime_error(
                      "MultihostMulticoreComputeService::processWorkCompletion(): couldn't find non-ready child in non-ready set!");
            }


//            if (this->non_ready_workunits.find(child) == this->non_ready_workunits.end()) {
//              throw std::runtime_error(
//                      "MultihostMulticoreComputeService::processWorkCompletion(): can't find non-ready child in non-ready set!");
//            }
//            this->non_ready_workunits.erase(child);
//            this->ready_workunits.insert(child);
          }
        }
      }

      this->releaseDaemonLock();

    }


/**
 * @brief Process a work failure
 * @param worker_thread: the worker thread that did the work
 * @param workunit: the workunit
 * @param cause: the cause of the failure
 */

    void StandardJobExecutor::processWorkunitExecutorFailure(
            WorkunitMulticoreExecutor *workunit_executor,
            Workunit *workunit,
            std::shared_ptr<FailureCause> cause) {


      // Don't kill me while I am doing this
      this->acquireDaemonLock();

      WRENCH_INFO("A workunit executor has failed to complete a workunit on behalf of job '%s'", this->job->getName().c_str());

      // Update core availabilities
      this->core_availabilities[workunit_executor->getHostname()] += workunit_executor->getNumCores();
      // Update RAM availabilities
      this->ram_availabilities[workunit_executor->getHostname()] += workunit_executor->getMemoryUtilization();

      // Remove the workunit executor from the workunit executor list and put it in the failed list
      for (auto it = this->running_workunit_executors.begin(); it != this->running_workunit_executors.end(); it++) {
        if ((*it).get() == workunit_executor) {
          PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_workunit_executors), &(this->failed_workunit_executors));
          break;
        }
      }

      // Remove the work from the running work queue
      bool found_it = false;
      for (auto it = this->running_workunits.begin(); it != this->running_workunits.end(); it++) {
        if ((*it).get() == workunit) {
          this->running_workunits.erase(it);
          found_it = true;
          break;
        }
      }
      if (!found_it) {
        throw std::runtime_error(
                "StandardJobExecutor::processWorkunitExecutorCompletion(): couldn't find a recently failed workunit in the running workunit list");
      }

      // Remove all other workunits for the job in the "not ready" state
      this->non_ready_workunits.clear();

      // Remove all other workunits for the job in the "ready" state
      this->ready_workunits.clear();

      // Deal with running workunits!
      for (auto const &wu : this->running_workunits) {
        if ((not wu->post_file_copies.empty()) || (not wu->pre_file_copies.empty())) {
          throw std::runtime_error(
                  "StandardJobExecutor::processWorkunitExecutorFailure(): trying to cancel a running workunit that's doing some file copy operations - not supported (for now)");
        }
        // find the workunit executor  that's doing the work (lame iteration)
        for (auto const &wue : this->running_workunit_executors) {
          if (wue->workunit == wu.get()) {
            wue->kill();
            break;
          }
        }
      }
      this->running_workunits.clear();

      // Deal with completed workunits
      this->completed_workunits.clear();


      // Send the notification back
      try {
        S4U_Mailbox::putMessage(this->callback_mailbox,
                                new StandardJobExecutorFailedMessage(this->job, this, cause,
                                                                     this->getPropertyValueAsDouble(
                                                                             StandardJobExecutorProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        // do nothing
      }

      this->releaseDaemonLock();

    }



/**
 * @brief Create all work for a newly dispatched job
 * @param job: the job
 */
    void StandardJobExecutor::createWorkunits() {

      Workunit *pre_file_copies_work_unit = nullptr;
      std::vector<Workunit *> task_work_units;
      Workunit *post_file_copies_work_unit = nullptr;
      Workunit *cleanup_workunit = nullptr;

      // Create the cleanup workunit, if any
      if (not job->cleanup_file_deletions.empty()) {
        cleanup_workunit = new Workunit({}, {}, {}, {}, job->cleanup_file_deletions);
      }

      // Create the pre_file_copies work unit, if any
      if (not job->pre_file_copies.empty()) {
        pre_file_copies_work_unit = new Workunit(job->pre_file_copies, {}, {}, {}, {});
      }

      // Create the post_file_copies work unit, if any
      if (not job->post_file_copies.empty()) {
        post_file_copies_work_unit = new Workunit({}, {}, {}, job->post_file_copies, {});
      }

      // Create the task work units, if any
      for (auto const &task : job->tasks) {
        task_work_units.push_back(new Workunit({}, {task}, job->file_locations, {}, {}));
      }

      // Add dependencies from pre copies to possible successors
      if (pre_file_copies_work_unit != nullptr) {
        if (not task_work_units.empty()) {
          for (auto const &twu: task_work_units) {
            Workunit::addDependency(pre_file_copies_work_unit, twu);
          }
        } else if (post_file_copies_work_unit != nullptr) {
          Workunit::addDependency(pre_file_copies_work_unit, post_file_copies_work_unit);
        } else if (cleanup_workunit != nullptr) {
          Workunit::addDependency(pre_file_copies_work_unit, cleanup_workunit);
        }
      }

      // Add dependencies from tasks to possible successors
      for (auto const &twu: task_work_units) {
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
      std::vector<Workunit*> all_work_units;
      if (pre_file_copies_work_unit) all_work_units.push_back(pre_file_copies_work_unit);
      for (auto const &twu : task_work_units) {
        all_work_units.push_back(twu);
      }
      if (post_file_copies_work_unit) all_work_units.push_back(post_file_copies_work_unit);
      if (cleanup_workunit) all_work_units.push_back(cleanup_workunit);

      task_work_units.clear();

      // Insert work units in the ready or non-ready queues
      for (auto const &wu : all_work_units) {
        if (wu->num_pending_parents == 0) {
          this->ready_workunits.insert(std::unique_ptr<Workunit>(wu));
        } else {
          this->non_ready_workunits.insert(std::unique_ptr<Workunit>(wu));
        }
      }

      all_work_units.clear();
    }


    /**
     * @brief Sort the a list of ready workunits based on the TASK_SELECTION_ALGORITHM property
     *
     * @return a sorted vector of ready tasks
     */
    std::vector<Workunit*> StandardJobExecutor::sortReadyWorkunits() {

//      std::cerr << "In sortReadyWorkunits()\n";

      std::vector<Workunit *> sorted_workunits;

      for (auto const &wu : this->ready_workunits) {
        sorted_workunits.push_back(wu.get());
//        std::cerr << "WORKUNITS.GET = " << wu.get() << ": " << wu.get()->tasks.size() << "\n";
      }

//      std::cerr << "SORTED LENGTH = " << sorted_workunits.size() << "\n";

      std::string selection_algorithm =
              this->getPropertyValueAsString(StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM);

//      std::cerr << "SELECT ALG = " << selection_algorithm << "\n";

      // using function as comp
      std::sort(sorted_workunits.begin(), sorted_workunits.end(),
                [selection_algorithm](const Workunit*  wu1, const Workunit*  wu2) -> bool
                {
//                    std::cerr << "IN LAMBDA1: " << wu1 << "  " << wu2 << "\n";
//                    std::cerr << "IN LAMBDA2: " << wu1->tasks.size() << "  " << wu2->tasks.size() << "\n";
                    // Non-computational workunits have higher priority

                    if (wu1->tasks.empty() and wu2->tasks.empty()) {
                      return ((uintptr_t) wu1 > (uintptr_t) wu2);
                    }
                    if (wu1->tasks.empty()) {
                      return true;
                    }
                    if (wu2->tasks.empty()) {
                      return false;
                    }

                    if (selection_algorithm == "maximum_flops") {
                      if (wu1->tasks[0]->getFlops() == wu2->tasks[0]->getFlops()) {
                        return ((uintptr_t) wu1 > (uintptr_t) wu2);
                      }
                      return (wu1->tasks[0]->getFlops() >= wu2->tasks[0]->getFlops());
                    } else if (selection_algorithm == "maximum_minimum_cores") {
                      if (wu1->tasks[0]->getMinNumCores() == wu2->tasks[0]->getMinNumCores()) {
                        return ((uintptr_t) wu1 > (uintptr_t) wu2);
                      }
                      return (wu1->tasks[0]->getMinNumCores() >= wu2->tasks[0]->getMinNumCores());
                    } else {
                      throw std::runtime_error("Unknown StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM property '"
                                               + selection_algorithm + "'");
                    }
                });

      return sorted_workunits;
    }


    /**
     * @brief Retrieve the executor's job
     * @return a standard job
     */
    StandardJob *StandardJobExecutor::getJob() {
      return this->job;
    }

    /**
     * @brief Retrieve the executor's compute resources
     * @return a set of compute resources
     */
    std::set<std::tuple<std::string, unsigned long, double>>  StandardJobExecutor::getComputeResources() {
      return this->compute_resources;
    }


};

