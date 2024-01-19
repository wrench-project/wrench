/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <string>
#include <utility>
#include <wrench-dev.h>

WRENCH_LOG_CATEGORY(wrench_core_workflow_job, "Log category for Job");


namespace wrench {

    /**
     * @brief Destructor
     */
    Job::~Job() {
    }

    /**
     * @brief Constructor
     *
     * @param name: job name (if empty, a name will be chosen for you)
     * @param job_manager: manager in charge of this job
     */
    Job::Job(std::string name, std::shared_ptr<JobManager> job_manager) {
        if (name.empty()) {
            this->name = "job_" + std::to_string(getNewUniqueNumber());
        } else {
            this->name = name;
        }
        this->job_manager = std::move(job_manager);
        this->originator_commport = this->job_manager->getCreatorCommPort();

        this->parent_compute_service = nullptr;
        this->submit_date = -1.0;
        this->end_date = -1.0;
    }

    /**
     * @brief Get the job's name
     *
     * @return the name as a string
     */
    std::string Job::getName() {
        return this->name;
    }

    /**
     * @brief Get the "origin" callback commport
     *
     * @return the next callback commport
     */
    S4U_CommPort *Job::getOriginCallbackCommPort() {
        return this->originator_commport;
    }


    /**
     * @brief Method to print the call back stack
     */
    void Job::printCallbackCommPortStack() {
        auto mystack = this->callback_commport_stack;
        while (not mystack.empty()) {
            WRENCH_INFO("   STACK : %s", mystack.top()->get_cname());
            mystack.pop();
        }
        WRENCH_INFO("   ORIGINAL : %s", this->originator_commport->get_cname());
    }

    /**
     * @brief Get the "next" callback commport (returns the
     *         workflow commport if the commport stack is empty), and
     *         pops it
     *
     * @return the next callback commport
     */
    S4U_CommPort *Job::popCallbackCommPort() {
        if (this->callback_commport_stack.empty()) {
            return this->originator_commport;
        }
        auto commport = this->callback_commport_stack.top();
        this->callback_commport_stack.pop();
        return commport;
    }

    /**
     * @brief Get the job's "next" callback commport, without popping it
     * @return the next callback commport
     */
    S4U_CommPort *Job::getCallbackCommPort() {
        if (this->callback_commport_stack.empty()) {
            return this->originator_commport;
        }
        return this->callback_commport_stack.top();
    }

    /**
     * @brief Pushes a callback commport
     *
     * @param commport: the commport name
     */
    void Job::pushCallbackCommPort(S4U_CommPort *commport) {
        this->callback_commport_stack.push(commport);
    }

    /**
     * @brief Generate a unique number (for each newly generated job)
     *
     * @return a unique number
     */
    unsigned long Job::getNewUniqueNumber() {
        static unsigned long sequence_number = 0;
        return (sequence_number++);
    }

    /**
    * @brief Set the parent compute service of the job
    * @param compute_service: a compute service
    */
    void Job::setParentComputeService(std::shared_ptr<ComputeService> compute_service) {
        this->parent_compute_service = std::move(compute_service);
    }

    /**
     * @brief Get the compute service that is running /ran the job
     *
     * @return a compute service
     */
    std::shared_ptr<ComputeService> Job::getParentComputeService() {
        return this->parent_compute_service;
    }

    /**
     * @brief Get the date at which the job was last submitted (<0 means "never submitted")
     * @return the submit date
     */
    double Job::getSubmitDate() const {
        return this->submit_date;
    }

    /**
    * @brief Get the date at which the job ended (<0 means "never submitted")
    * @return the end date
    */
    double Job::getEndDate() const {
        return this->end_date;
    }

    /**
     * @brief Return the service-specific arguments that are used during job submission
     * @return a map of argument name/values
     */
    std::map<std::string, std::string> &Job::getServiceSpecificArguments() {
        return this->service_specific_args;
    }

    /**
     * @brief Sets the service-specific arguments that are used during job submission
     * @param args: a map of argument name/values
     */
    void Job::setServiceSpecificArguments(std::map<std::string, std::string> args) {
        this->service_specific_args = std::move(args);
    }

    /**
    * @brief Get the job's priority (the higher the priority value, the higher the priority)
    * @return the job's priority
    */
    double Job::getPriority() const {
        return this->priority;
    }

    /**
   * @brief Set the job's priority (the higher the priority value, the higher the priority)
   * @param p: a priority
   */
    void Job::setPriority(double p) {
        this->priority = p;
    }
}// namespace wrench
