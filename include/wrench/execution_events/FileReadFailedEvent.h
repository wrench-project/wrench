/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILE_READ_FAILED_EVENT_H
#define WRENCH_FILE_READ_FAILED_EVENT_H

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
     * @brief A "file read has failed" ExecutionEvent
     */
    class FileReadFailedEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param location: the location
         * @param num_bytes: the number of bytes to read
         * @param failure_cause: a failure cause
         */
        FileReadFailedEvent(std::shared_ptr<FileLocation> location,
                            sg_size_t num_bytes,
                            std::shared_ptr<FailureCause> failure_cause)
            : location(std::move(location)), num_bytes(num_bytes),
              failure_cause(std::move(failure_cause)) {}

    public:
        /** @brief The location */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of bytes that should have been read */
        sg_size_t num_bytes;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> failure_cause;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override {
            return "FileReadFailedEvent (file: " + this->location->getFile()->getID() +
                   "; location = " + this->location->toString() +
                   "; num_bytes = " + std::to_string(this->num_bytes) +
                   "; cause: " + this->failure_cause->toString() + ")";
        }
    };


}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FILE_READ_FAILED_EVENT_H
