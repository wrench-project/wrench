/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILE_COPY_FAILED_EVENT_H
#define WRENCH_FILE_COPY_FAILED_EVENT_H

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
    class FileCopyFailedEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param src: source location
         * @param dst: destination location
         * @param failure_cause: a failure cause
         */
        FileCopyFailedEvent(std::shared_ptr<FileLocation> src,
                            std::shared_ptr<FileLocation> dst,
                            std::shared_ptr<FailureCause> failure_cause)
            : src(std::move(src)), dst(std::move(dst)),
              failure_cause(std::move(failure_cause)) {}

    public:
        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> failure_cause;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override {
            return "FileCopyFailedEvent (file: " + this->src->getFile()->getID() +
                   "; src = " + this->src->toString() +
                   "; dst = " + this->dst->toString() +
                   "; cause: " + this->failure_cause->toString() + ")";
        }
    };


}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FILE_COPY_FAILED_EVENT_H
