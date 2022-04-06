/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_JOB_H
#define WRENCH_JOB_H


#include <string>
#include <stack>
#include <map>
#include <memory>

#include "wrench/managers/JobManager.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class Workflow;
    class ComputeService;


    /**
     * @brief Abstraction of a job used for executing tasks in a Workflow
     */
    class Job {

    public:
        std::string getName();

        double getSubmitDate();
        double getEndDate();

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        simgrid::s4u::Mailbox *popCallbackMailbox();

        void printCallbackMailboxStack();

        void pushCallbackMailbox(simgrid::s4u::Mailbox *mailbox);

        simgrid::s4u::Mailbox *getCallbackMailbox();

        simgrid::s4u::Mailbox *getOriginCallbackMailbox();

        void setParentComputeService(std::shared_ptr<ComputeService> compute_service);

        std::shared_ptr<ComputeService> getParentComputeService();


        virtual ~Job();

        virtual void setPriority(double p);

        double getPriority() const;

        std::map<std::string, std::string> &getServiceSpecificArguments();

    protected:
        friend class JobManager;

        Job(std::string name, std::shared_ptr<JobManager> job_manager);

        void setServiceSpecificArguments(std::map<std::string, std::string> args);

        unsigned long getNewUniqueNumber();

        /** @brief Service-specific arguments used during job submission **/
        std::map<std::string, std::string> service_specific_args;

        /** @brief Stack of callback mailboxes (to pop notifications) */
        std::stack<simgrid::s4u::Mailbox *> callback_mailbox_stack;
        /** @brief The Job Manager in charge of this job **/
        std::shared_ptr<JobManager> job_manager;
        /** @brief The originator's mailbox */
        simgrid::s4u::Mailbox *originator_mailbox;

        /** @brief The job's name */
        std::string name;
        /** @brief The date at which the job was last submitted */
        double submit_date;
        /** @brief The date at which the job ended (with success or failure) */
        double end_date;
        /** @brief The compute service to which the job was submitted */
        std::shared_ptr<ComputeService> parent_compute_service;

        /** @brief Whether the job has already been submitted to the job manager */
        bool already_submitted_to_job_manager = false;

        /** @brief The job's priority (the higher the number, the higher the priority) */
        double priority = 0.0;

    private:
        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

};// namespace wrench


#endif//WRENCH_JOB_H
