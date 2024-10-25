/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILE_READ_COMPLETED_EVENT_H
#define WRENCH_FILE_READ_COMPLETED_EVENT_H

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
     * @brief A "file read has completed" ExecutionEvent
     */
    class FileReadCompletedEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param location: the location
         * @param num_bytes: the number of bytes read
         */
        FileReadCompletedEvent(std::shared_ptr<FileLocation> location,
                               sg_size_t num_bytes)
            : location(std::move(location)), num_bytes(num_bytes) {}

    public:
        /** @brief The  location */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of bytes read */
        sg_size_t num_bytes;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override {
            return "FileReadCompletedEvent (file: " + this->location->getFile()->getID() +
                   "; location = " + this->location->toString() + ")";
        }
    };


}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FILE_READ_COMPLETED_EVENT_H
