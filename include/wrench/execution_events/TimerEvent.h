/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_TIMER_EVENT_H
#define WRENCH_TIMER_EVENT_H

#include <string>
#include <utility>
#include "wrench/failure_causes/FailureCause.h"

/***********************/
/** \cond DEVELOPER    */
/***********************/

namespace wrench {

    class WorkflowTask;

    class DataFile;

    class StandardJob;

    class PilotJob;

    class ComputeService;

    class StorageService;

    class FileRegistryService;

    class FileRegistryService;

    /**
     * @brief A "timer went off" ExecutionEvent
     */
    class TimerEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param message: some arbitrary message
         */
        explicit TimerEvent(std::string message)
            : message(std::move(message)) {}

    public:
        /** @brief The message */
        std::string message;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override { return "TimerEvent (message: " + this->message + ")"; }
    };

}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_TIMER_EVENT_H
