/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <string>
#include <wrench-dev.h>
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/workflow/Workflow.h"

WRENCH_LOG_CATEGORY(wrench_core_workflow_job, "Log category for WorkflowJob");


namespace wrench {

    /**
     * @brief Destructor
     */
    WorkflowJob::~WorkflowJob() {
    }

    /**
     * @brief Constructor
     *
     * @param type: job type
     */
    WorkflowJob::WorkflowJob() {
      this->parent_compute_service = nullptr;
      this->submit_date = -1.0;
    }

    /**
     * @brief Get the job's name
     *
     * @return the name as a string
     */
    std::string WorkflowJob::getName() {
      return this->name;
    }

    /**
     * @brief Get the "next" callback mailbox (returns the
     *         origin (i.e., workflow) mailbox if the mailbox stack is empty)
     *
     * @return the next callback mailbox
     */
    std::string WorkflowJob::getCallbackMailbox() {
      if (this->callback_mailbox_stack.empty()) {
        return this->workflow->getCallbackMailbox();
      }
      return this->callback_mailbox_stack.top();
    }

    /**
     * @brief Get the "origin" callback mailbox
     *
     * @return the next callback mailbox
     */
    std::string WorkflowJob::getOriginCallbackMailbox() {
      return this->workflow->getCallbackMailbox();
    }


    /**
     * @brief Get the "next" callback mailbox (returns the
     *         workflow mailbox if the mailbox stack is empty), and
     *         pops it
     *
     * @return the next callback mailbox
     */
    std::string WorkflowJob::popCallbackMailbox() {
      if (this->callback_mailbox_stack.empty()) {
        return this->workflow->getCallbackMailbox();
      }
      std::string mailbox = this->callback_mailbox_stack.top();
      this->callback_mailbox_stack.pop();
      return mailbox;
    }

    /**
     * @brief Pushes a callback mailbox
     *
     * @param mailbox: the mailbox name
     */
    void WorkflowJob::pushCallbackMailbox(std::string mailbox) {
      this->callback_mailbox_stack.push(mailbox);
    }

    /**
     * @brief Generate a unique number (for each newly generated job)
     *
     * @return a unique number
     */
    unsigned long WorkflowJob::getNewUniqueNumber() {
      static unsigned long sequence_number = 0;
      return (sequence_number++);
    }

    /**
    * @brief Set the parent compute service of the job
    * @param compute_service: a compute service
    */
    void WorkflowJob::setParentComputeService(std::shared_ptr<ComputeService> compute_service) {
      this->parent_compute_service = compute_service;
    }

    /**
     * @brief Get the parent compute service of the job
     *
     * @return a compute service
     */
    std::shared_ptr<ComputeService> WorkflowJob::getParentComputeService() {
      return this->parent_compute_service;
    }

    /**
     * @brief Get the date at which the job was last submitted (<0 means "never submitted")
     * @return the submit date
     */
    double WorkflowJob::getSubmitDate() {
      return this->submit_date;
    }

    /**
     * @brief Return the service-specific arguments that were used during job submission
     * @return a map of argument name/values
     */
    std::map<std::string, std::string> WorkflowJob::getServiceSpecificArguments() {
      return this->service_specific_args;
    }

    /**
     * @brief Return default job priority as zero.
     * @return priority as zero
     */
    unsigned long WorkflowJob::getPriority() {
      return 0;
    }
}
