/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILE_COPY_COMPLETED_EVENT_H
#define WRENCH_FILE_COPY_COMPLETED_EVENT_H

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
    class FileCopyCompletedEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param file: a workflow file
         * @param src: the source location
         * @param dst: the destination location
         */
        FileCopyCompletedEvent(std::shared_ptr<DataFile> file,
                               std::shared_ptr<FileLocation> src,
                               std::shared_ptr<FileLocation> dst)
            : file(std::move(file)), src(std::move(src)), dst(std::move(dst)) {}

    public:
        /** @brief The workflow file that has successfully been copied */
        std::shared_ptr<DataFile> file;
        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override {
            return "FileCopyCompletedEvent (file: " + this->file->getID() +
                   "; src = " + this->src->toString() +
                   "; dst = " + this->dst->toString();
        }
    };


}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_FILE_COPY_COMPLETED_EVENT_H
