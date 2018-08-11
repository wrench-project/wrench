/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WORKFLOWJOB_H
#define WRENCH_WORKFLOWJOB_H


#include <string>
#include <stack>

namespace wrench {


    class Workflow;
    class ComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Abstraction of a job used for executing tasks in a Workflow
     */
    class WorkflowJob {
    public:

        /** @brief Job types */
        enum Type {
            /** @brief A standard job that can be submitted directly to a ComputeService for execution */
            STANDARD,
            /** @brief A pilot job that can be submitted to a ComputeService and that, once started, will act as a ComputeService
             * (likely a MultihostMulticoreComputeService) with an expiration date */
            PILOT
        };

        Type getType();

        std::string getTypeAsString();

        std::string getName();

        double getSubmitDate();

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        void setSubmitDate(double date);

        std::string popCallbackMailbox();

        void pushCallbackMailbox(std::string);

        std::string getCallbackMailbox();

        std::string getOriginCallbackMailbox();

        void setParentComputeService(ComputeService *compute_service);

        ComputeService *getParentComputeService();

        virtual ~WorkflowJob();

    protected:

        WorkflowJob(Type type);

        unsigned long getNewUniqueNumber();

        /** @brief Stack of callback mailboxes (to pop notifications) */
        std::stack<std::string> callback_mailbox_stack;
        /** @brief The workflow this job belong to */
        Workflow *workflow;
        /** @brief The job's type */
        Type type;
        /** @brief The job's name */
        std::string name;
        /** @brief The date at which the job was last submitted */
        double submit_date;
        /** @brief The compute service to which the job was submitted */
        ComputeService *parent_compute_service;

    private:
        bool forward_notification_to_original_source;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKFLOWJOB_H
