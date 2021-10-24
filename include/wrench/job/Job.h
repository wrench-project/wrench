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


    class Workflow;
    class ComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

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

        std::string popCallbackMailbox();

        void pushCallbackMailbox(std::string);

        std::string getCallbackMailbox();

        std::string getOriginCallbackMailbox();

        void setParentComputeService(std::shared_ptr<ComputeService> compute_service);

        std::shared_ptr<ComputeService> getParentComputeService();

        std::map<std::string, std::string> getServiceSpecificArguments();

        virtual unsigned long getPriority();

        virtual ~Job();

    protected:

        friend class JobManager;

        Job(std::string name, std::shared_ptr<JobManager> job_manager);

        unsigned long getNewUniqueNumber();

        /** @brief Service-specific arguments used during job submission **/
        std::map<std::string, std::string> service_specific_args;

        /** @brief Stack of callback mailboxes (to pop notifications) */
        std::stack<std::string> callback_mailbox_stack;
        /** @brief The Job Manager in charge of this job **/
        std::shared_ptr<JobManager> job_manager;
        /** @brief The workflow this job belong to */
        Workflow *workflow;
        /** @brief The job's name */
        std::string name;
        /** @brief The date at which the job was last submitted */
        double submit_date;
        /** @brief The date at which the job ended (with success or failure) */
        double end_date;
        /** @brief The compute service to which the job was submitted */
        std::shared_ptr<ComputeService> parent_compute_service;


    private:
//        bool forward_notification_to_original_source;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_JOB_H
