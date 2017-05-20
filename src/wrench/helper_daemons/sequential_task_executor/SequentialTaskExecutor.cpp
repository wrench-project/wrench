/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <simgrid_S4U_util/S4U_Simulation.h>
#include <logging/TerminalOutput.h>
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include <services/storage_services/StorageService.h>
#include <workflow/WorkflowTask.h>
#include <simulation/SimulationMessage.h>
#include <services/file_registry_service/FileRegistryService.h>
#include <exceptions/WorkflowExecutionException.h>
#include <services/ServiceMessage.h>

#include "SequentialTaskExecutor.h"
#include "SequentialTaskExecutorMessage.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor, "Log category for Sequential Task Executor");

namespace wrench {

    /**
     * @brief Constructor, which starts the daemon for the service on a host
     *
     * @param hostname: the name of the host
     * @param callback_mailbox: the callback mailbox to which the sequential
     *        task executor sends back "task done" or "task failed" messages
     * @param default_storage_service: a raw pointer to the default StorageService (if any)
     * @param file_registry: a raw pointer to a FileRegistryService object
     * @param task_startup_overhead: the task startup overhead, in seconds
     */
    SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname, std::string callback_mailbox,
                                                   StorageService *default_storage_service,
                                                   FileRegistryService *file_registry,
                                                   double task_startup_overhead) :
            S4U_DaemonWithMailbox("sequential_task_executor", "sequential_task_executor") {

      if (task_startup_overhead < 0) {
        throw std::invalid_argument("SequentialTaskExecutor::SequentialTaskExecutor(): task startup overhead must be >0");
      }
      this->hostname = hostname;
      this->callback_mailbox = callback_mailbox;
      this->task_start_up_overhead = task_startup_overhead;
      this->default_storage_service = default_storage_service;
      this->file_registry = file_registry;

      // Start my daemon on the host
      this->start(this->hostname);
    }

    /**
     * @brief Terminate the sequential task executor
     */
    void SequentialTaskExecutor::stop() {
      // Send a termination message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
    }

    /**
     * @brief Kill the sequential task executor
     */
    void SequentialTaskExecutor::kill() {
      this->kill_actor();
    }

    /**
     * @brief Have the sequential task executor a task
     *
     * @param task: a pointer to the task
     *
     * @return 0 on success
     */
    int SequentialTaskExecutor::runTask(WorkflowTask *task, std::map<WorkflowFile*, StorageService*> file_locations) {
      if (task == nullptr) {
        throw std::invalid_argument("SequentialTaskExecutor::runTask(): passed a nullptr task");
      }

      // Send a "run a task" message to the daemon's mailbox
      S4U_Mailbox::put(this->mailbox_name, new SequentialTaskExecutorRunTaskMessage(task, file_locations, 0.0));
      return 0;
    };




    /**
     * @brief Main method of the sequential task executor daemon
     *
     * @return 0 on termination
     *
     * @throw std::runtime_error
     */
    int SequentialTaskExecutor::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_BLUE);

      WRENCH_INFO("New Sequential Task Executor starting (%s) ", this->mailbox_name.c_str());

      while (true) {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

        if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage*>(message.get())) {
          if (msg->ack_mailbox != "") {
            S4U_Mailbox::put(msg->ack_mailbox, new ServiceDaemonStoppedMessage(0.0));
          }
          break;

        } else if (SequentialTaskExecutorRunTaskMessage *msg = dynamic_cast<SequentialTaskExecutorRunTaskMessage*>(message.get())) {

          // Download  all input files
          try {
            StorageService::downloadFiles(msg->task->getInputFiles(),
                                          msg->file_locations,
                                          this->default_storage_service);
          } catch (WorkflowExecutionException &e) {

            WRENCH_INFO("Notifying mailbox %s that task %s has failed",
                        this->callback_mailbox.c_str(),
                        msg->task->getId().c_str());
            S4U_Mailbox::dput(this->callback_mailbox,
                              new SequentialTaskExecutorTaskFailedMessage(msg->task, this, e.getCause(), 0.0));
            continue;
          }

          // Run the task
          WRENCH_INFO("Executing task %s (%lf flops)", msg->task->getId().c_str(), msg->task->getFlops());
          msg->task->setRunning();
          S4U_Simulation::sleep(this->task_start_up_overhead);
          S4U_Simulation::compute(msg->task->getFlops());

          // Upload all output files
          try {
            StorageService::uploadFiles(msg->task->getOutputFiles(), msg->file_locations, this->default_storage_service);
          } catch (WorkflowExecutionException &e) {
            WRENCH_INFO("Notifying mailbox %s that task %s has failed due to output files problems",
                        this->callback_mailbox.c_str(),
                        msg->task->getId().c_str());
            S4U_Mailbox::dput(this->callback_mailbox,
                              new SequentialTaskExecutorTaskFailedMessage(msg->task, this, e.getCause(), 0.0));
            continue;
          }

          // Set the task completion time and state
          msg->task->setEndDate(S4U_Simulation::getClock());


          // Send the callback
          WRENCH_INFO("Notifying mailbox %s that task %s has finished",
                      this->callback_mailbox.c_str(),
                      msg->task->getId().c_str());
          S4U_Mailbox::dput(this->callback_mailbox,
                            new SequentialTaskExecutorTaskDoneMessage(msg->task, this, 0.0));

        } else {
          throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
      }

      WRENCH_INFO("Sequential Task Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

};
