/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILE_WRITE_COMPLETED_EVENT_H
#define WRENCH_FILE_WRITE_COMPLETED_EVENT_H

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
     * @brief A "file copy has completed" ExecutionEvent
     */
    class FileWriteCompletedEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param location: the write location
         */
        FileWriteCompletedEvent(std::shared_ptr<FileLocation> location)
            : location(std::move(location)) {}

    public:
        /** @brief The location */
        std::shared_ptr<FileLocation> location;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override {
            return "FileWriteCompletedEvent (file: " + this->location->getFile()->getID() +
                   "; location = " + this->location->toString() + ")";
        }
    };


}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FILE_WRITE_COMPLETED_EVENT_H
