/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILE_WRITE_FAILED_EVENT_H
#define WRENCH_FILE_WRITE_FAILED_EVENT_H

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
     * @brief A "file copy has failed" ExecutionEvent
     */
    class FileWriteFailedEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param location: the location
         * @param failure_cause: a failure cause
         */
        FileWriteFailedEvent(std::shared_ptr<FileLocation> location,
                             std::shared_ptr<FailureCause> failure_cause)
            : location(std::move(location)),
              failure_cause(std::move(failure_cause)) {}

    public:
        /** @brief The  location */
        std::shared_ptr<FileLocation> location;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> failure_cause;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override {
            return "FileWriteFailedEvent (file: " + this->location->getFile()->getID() +
                   "; location = " + this->location->toString() +
                   "; cause: " + this->failure_cause->toString() + ")";
        }
    };


}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FILE_WRITE_FAILED_EVENT_H
