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

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Abstraction of a job used for executing tasks in a Workflow
     */
    class WorkflowJob {
    public:

        /** @brief Job type enum */
        enum Type {
            STANDARD,
            PILOT
        };

        Type getType();

        std::string getTypeAsString();

        std::string getName();

        int getNumCores();

        double getDuration();

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        std::string popCallbackMailbox();

        void pushCallbackMailbox(std::string);

        std::string getCallbackMailbox();

        std::string getOriginCallbackMailbox();

    protected:

        WorkflowJob(Type type, unsigned long num_cores, double duration);

        unsigned long getNewUniqueNumber();

        /** @brief Stack of callback mailboxes (to pop notifications) */
        std::stack<std::string> callback_mailbox_stack;
        /** @brief The workflow this job belong to */
        Workflow *workflow;
        /** @brief The job's type */
        Type type;
        /** @brief The job's name */
        std::string name;
        /** @brief The job's duration */
        double duration;
        /** @brief The job's number of cores */
        unsigned long num_cores;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKFLOWJOB_H
