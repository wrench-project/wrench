/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <services/storage_services/StorageService.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include <logging/TerminalOutput.h>
#include <exceptions/WorkflowExecutionException.h>
#include <simgrid_S4U_util/S4U_Simulation.h>
#include "WorkunitMulticoreExecutor.h"
#include "StandardJobExecutorMessage.h"
#include <workflow/WorkflowTask.h>
#include <workflow_job/StandardJob.h>
#include <simulation/SimulationTimestampTypes.h>
#include "Workunit.h"
#include <simulation/Simulation.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(workunit_multicore_executor, "Log category for Worker Thread");


namespace wrench {

    /**
     * @brief Constructor, which starts the workunit executor on the host
     *
     * @param simulation: the simulation
     * @param hostname: the name of the host
     * @param num_cores: the number of cores available to the executor
     * @param callback_mailbox: the callback mailbox to which the worker
     *        thread can send "work done" messages
     * @param workunit: the workinit to perform
     * @param default_storage_service: the default storage service from which to read/write data (if any)
     * @param startup_overhead: the startup overhead, in seconds
     */
    WorkunitMulticoreExecutor::WorkunitMulticoreExecutor(
            Simulation *simulation,
            std::string hostname,
            unsigned long num_cores,
            std::string callback_mailbox,
            std::shared_ptr<Workunit> workunit,
            StorageService *default_storage_service,
            double startup_overhead) :
            S4U_DaemonWithMailbox("workunit_multicore_executor", "workunit_multicore_executor") {

      if (startup_overhead < 0) {
        throw std::invalid_argument("WorkunitMulticoreExecutor::WorkunitMulticoreExecutor(): startup_overhead must be >= 0");
      }
      if (num_cores < 1) {
        throw std::invalid_argument("WorkunitMulticoreExecutor::WorkunitMulticoreExecutor(): num_cores must be >= 1");
      }

      this->simulation = simulation;
      this->hostname = hostname;
      this->callback_mailbox = callback_mailbox;
      this->workunit = workunit;
      this->start_up_overhead = startup_overhead;
      this->num_cores = num_cores;
      this->default_storage_service = default_storage_service;

      // Start my daemon on the host
      this->start(this->hostname);
    }

    /**
     * @brief Kill the worker thread
     */
    void WorkunitMulticoreExecutor::kill() {
      // TODO: kill all worker threads...
      throw std::runtime_error("WorkunitMulticoreExecutor::kill(): not implemented yet");
      this->kill_actor();
    }


    /**
    * @brief Main method of the worker thread daemon
    *
    * @return 0 on termination
    *
    * @throw std::runtime_error
    */
    int WorkunitMulticoreExecutor::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_BLUE);

      WRENCH_INFO("New Worker Thread starting (%s) to do: %ld pre file copies, %ld tasks, %ld post file copies",
                  this->mailbox_name.c_str(),
                  this->workunit->pre_file_copies.size(),
                  this->workunit->tasks.size(),
                  this->workunit->post_file_copies.size());

      SimulationMessage *msg_to_send_back = nullptr;
      bool success;

      try {
        performWork(this->workunit);

        // build "success!" message
        success = true;
        msg_to_send_back = new WorkunitExecutorDoneMessage(
                this,
                this->workunit,
                0.0);

      } catch (WorkflowExecutionException &e) {
        // build "failed!" message
        success = false;
        msg_to_send_back = new WorkunitExecutorFailedMessage(
                this,
                this->workunit,
                e.getCause(),
                0.0);
      }

      // Send the callback
      if (success) {
        WRENCH_INFO("Notifying mailbox %s that work has completed",
                    this->callback_mailbox.c_str());
      } else {
        WRENCH_INFO("Notifying mailbox %s that work has failed",
                    this->callback_mailbox.c_str());
      }

      try {
        S4U_Mailbox::putMessage(this->callback_mailbox, msg_to_send_back);
      } catch (std::shared_ptr<NetworkError> cause) {
        WRENCH_INFO("Work unit executor on host %s can't report back due to network error!", S4U_Simulation::getHostName().c_str());
        return 0;
      }

      WRENCH_INFO("Work unit executor on host %s terminating!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    /**
     * @brief Simulate work execution
     *
     * @param work: the work to perform
     */
    void
    WorkunitMulticoreExecutor::performWork(std::shared_ptr<Workunit> work) {

      // Simulate the startup overhead
      S4U_Simulation::sleep(this->start_up_overhead);

      /** Perform all pre file copies operations */
      // TODO: This is sequential right now, but probably it should be concurrent in some fashion
      for (auto file_copy : work->pre_file_copies) {
        WorkflowFile *file = std::get<0>(file_copy);
        StorageService *src = std::get<1>(file_copy);
        StorageService *dst = std::get<2>(file_copy);
        try {
          WRENCH_INFO("Copying file %s from %s to %s",
                      file->getId().c_str(),
                      src->getName().c_str(),
                      dst->getName().c_str());
          dst->copyFile(file, src);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

      /** Perform all tasks **/
      for (auto task : work->tasks) {

        // Read  all input files
        WRENCH_INFO("Reading the %ld input files for task %s", task->getInputFiles().size(), task->getId().c_str());
        try {
          StorageService::readFiles(task->getInputFiles(),
                                    work->file_locations,
                                    this->default_storage_service);
        } catch (WorkflowExecutionException &e) {
          throw;
        }

        // Run the task
        // TODO: The task could be multicore!!!
        WRENCH_INFO("Executing task %s (%lf flops)", task->getId().c_str(), task->getFlops());
        task->setRunning();
        task->setStartDate(S4U_Simulation::getClock());

        S4U_Simulation::compute(task->getFlops());

        WRENCH_INFO("Writing the %ld output files for task %s", task->getOutputFiles().size(), task->getId().c_str());

        // Write all output files
        try {
          StorageService::writeFiles(task->getOutputFiles(), work->file_locations, this->default_storage_service);
        } catch (WorkflowExecutionException &e) {
          throw;
        }

        task->setCompleted();

        task->setEndDate(S4U_Simulation::getClock());

        // Generate a SimulationTimestamp
        this->simulation->output.addTimestamp<SimulationTimestampTaskCompletion>(
                new SimulationTimestampTaskCompletion(task));
      }

      /** Perform all post file copies operations */
      // TODO: This is sequential right now, but probably it should be concurrent in some fashion
      for (auto file_copy : work->post_file_copies) {
        WorkflowFile *file = std::get<0>(file_copy);
        StorageService *src = std::get<1>(file_copy);
        StorageService *dst = std::get<2>(file_copy);
        try {
          dst->copyFile(file, src);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

      /** Perform all cleanup file deletions */
      for (auto cleanup : work->cleanup_file_deletions) {
        WorkflowFile *file = std::get<0>(cleanup);
        StorageService *storage_service = std::get<1>(cleanup);
        try {
          storage_service->deleteFile(file);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

    }

};
