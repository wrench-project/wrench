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
#include "WorkerThread.h"
#include "MulticoreComputeServiceMessage.h"
#include <workflow/WorkflowTask.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(worker_thread, "Log category for Worker Thread");


namespace wrench {

    /**
     * @brief Constructor, which starts the worker thread on the host
     *
     * @param hostname: the name of the host
     * @param callback_mailbox: the callback mailbox to which the worker
     *        thread can sed "work done" messages
     * @param default_storage_service: the default storage service from which to read/write data (if any)
     * @param startup_overhead: the startup overhead, in seconds
     */
    WorkerThread::WorkerThread(std::string hostname, std::string callback_mailbox,
                               StorageService *default_storage_service,
                               double startup_overhead) :
            S4U_DaemonWithMailbox("worker_thread", "worker_thread") {

      if (startup_overhead < 0) {
        throw std::invalid_argument("WorkerThread::WorkerThread(): startup overhead must be >= 0");
      }
      this->hostname = hostname;
      this->callback_mailbox = callback_mailbox;
      this->start_up_overhead = startup_overhead;
      this->default_storage_service = default_storage_service;

      // Start my daemon on the host
      this->start(this->hostname);
    }

    /**
    * @brief Terminate the worker thread
    */
    void WorkerThread::stop() {
      // Send a termination message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
    }

    /**
     * @brief Kill the worker thread
     */
    void WorkerThread::kill() {
      this->kill_actor();
    }

    /**
     * @brief Asynchronously have the worker thread do some work
     *
     * @param job: the standard workflow job on behalf of which the work is done
     * @param pre_file_copies: a set of file copy actions to perform first
     * @param tasks: a set of tasks to execute in sequence
     * @param file_locations: locations where tasks should read/write files
     * @param post_file_copies: a set of file copy actions to perform last
     * @param cleanup_file_deletions: a set of file deletions to perform regardless of completion or failure
     *
     * @return
     */
    void WorkerThread::doWork(StandardJob *job,
                              std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                              std::vector<WorkflowTask *> tasks,
                              std::map<WorkflowFile *, StorageService *> file_locations,
                              std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies,
                              std::set<std::tuple<WorkflowFile *, StorageService *>> cleanup_file_deletions) {

      // Send a "do work" message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name,
                       new WorkerThreadDoWorkRequestMessage(job, pre_file_copies, tasks, file_locations, post_file_copies, cleanup_file_deletions,
                                                            0.0));

      return;
    }

    /**
    * @brief Main method of the worker thread daemon
    *
    * @return 0 on termination
    *
    * @throw std::runtime_error
    */
    int WorkerThread::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_BLUE);

      WRENCH_INFO("New Sequential Task Executor starting (%s) ", this->mailbox_name.c_str());

      while (true) {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

        if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
          if (msg->ack_mailbox != "") {
            S4U_Mailbox::put(msg->ack_mailbox, new ServiceDaemonStoppedMessage(0.0));
          }
          break;

        } else if (WorkerThreadDoWorkRequestMessage *msg = dynamic_cast<WorkerThreadDoWorkRequestMessage *>(message.get())) {

          SimulationMessage *msg_to_send_back = nullptr;
          bool success;

          try {
            performWork(msg->pre_file_copies, msg->tasks, msg->file_locations, msg->post_file_copies);

            // build "success!" message
            success = true;
            msg_to_send_back = new WorkerThreadWorkDoneMessage(
                    msg->job,
                    this,
                    msg->pre_file_copies,
                    msg->tasks,
                    msg->file_locations,
                    msg->post_file_copies,
                    msg->cleanup_file_deletions,
                    0.0);

          } catch (WorkflowExecutionException &e) {
            // build "failed!" message
            success = false;
            msg_to_send_back = new WorkerThreadWorkFailedMessage(
                                      msg->job,
                                      this,
                                      msg->pre_file_copies,
                                      msg->tasks,
                                      msg->file_locations,
                                      msg->post_file_copies,
                                      msg->cleanup_file_deletions,
                                      e.getCause(),
                                      0.0);
          }

          // Do the cleanup
          for (auto cleanup : msg->cleanup_file_deletions) {
            WorkflowFile *file = std::get<0>(cleanup);
            StorageService *storage_service = std::get<1>(cleanup);
            try {
              storage_service->deleteFile(file);
            } catch (WorkflowExecutionException &e) {
              // Ignore exceptions since files may not be created, etc.. 
            }
          }


          // Send the callback
          if (success) {
            WRENCH_INFO("Notifying mailbox %s that work has completed",
                        this->callback_mailbox.c_str());
          } else {
            WRENCH_INFO("Notifying mailbox %s that work has failed",
                        this->callback_mailbox.c_str());
          }

          S4U_Mailbox::dput(this->callback_mailbox, msg_to_send_back);


        } else {
          throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
      }

      WRENCH_INFO("Worker thread on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }


    /**
     * @brief Simulate work execution
     *
     * @param pre_file_copies: a set of file copy actions to perform first
     * @param tasks: a set of tasks to execute in sequence
     * @param file_locations: locations where tasks should read/write files
     * @param post_file_copies: a set of file copy actions to perform last
     */
    void
    WorkerThread::performWork(std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> pre_file_copies,
                              std::vector<WorkflowTask *> tasks,
                              std::map<WorkflowFile *, StorageService *> file_locations,
                              std::set<std::tuple<WorkflowFile *, StorageService *, StorageService *>> post_file_copies) {

      // Simulate the startup overhead
      S4U_Simulation::sleep(this->start_up_overhead);

      /** Perform all pre file copies operations */
      // TODO: This is sequential right now, but probably it should be concurrent in some fashion
      for (auto file_copy : pre_file_copies) {
        WorkflowFile *file = std::get<0>(file_copy);
        StorageService *src = std::get<1>(file_copy);
        StorageService *dst = std::get<2>(file_copy);
        try {
          dst->copyFile(file, src);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

      /** Perform all tasks **/
      for (auto task : tasks) {

        // Read  all input files
        try {
          StorageService::readFiles(task->getInputFiles(),
                                    file_locations,
                                    this->default_storage_service);
        } catch (WorkflowExecutionException &e) {
          throw;
        }

        // Run the task
        WRENCH_INFO("Executing task %s (%lf flops)", task->getId().c_str(), task->getFlops());
        task->setRunning();
        S4U_Simulation::compute(task->getFlops());
        task->setCompleted();

        // Write all output files
        try {
          StorageService::writeFiles(task->getOutputFiles(), file_locations, this->default_storage_service);
        } catch (WorkflowExecutionException &e) {
          throw;
        }

        task->setEndDate(S4U_Simulation::getClock());

      }

      /** Perform all post file copies operations */
      // TODO: This is sequential right now, but probably it should be concurrent in some fashion
      for (auto file_copy : post_file_copies) {
        WorkflowFile *file = std::get<0>(file_copy);
        StorageService *src = std::get<1>(file_copy);
        StorageService *dst = std::get<2>(file_copy);
        try {
          dst->copyFile(file, src);
        } catch (WorkflowExecutionException &e) {
          throw;
        }
      }

    }

};