/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_WORKFLOWEXECUTIONEVENT_H
#define WRENCH_WORKFLOWEXECUTIONEVENT_H

#include <string>
#include "wrench/workflow/failure_causes/FailureCause.h"

/***********************/
/** \cond DEVELOPER    */
/***********************/

namespace wrench {
    /**
     * @brief A class to represent the various execution events that
     * are relevant to the execution of a workflow.
     */
    class WorkflowExecutionEvent {

    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        static std::shared_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent(std::string);
        static std::shared_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent(std::string, double timeout);

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        virtual std::string toString() { return "Generic WorkflowExecutionEvent"; }

        virtual ~WorkflowExecutionEvent() = default;

    protected:
        WorkflowExecutionEvent() = default;

        /***********************/
        /** \endcond           */
        /***********************/

    };

};

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_WORKFLOWEXECUTIONEVENT_H
