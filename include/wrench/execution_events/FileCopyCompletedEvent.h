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
         * @param file_registry_service: a file registry service
         * @param file_registry_service_updated: whether the file registry service has been updated
         */
        FileCopyCompletedEvent(std::shared_ptr<DataFile> file,
                               std::shared_ptr<FileLocation> src,
                               std::shared_ptr<FileLocation> dst,
                               std::shared_ptr<FileRegistryService> file_registry_service,
                               bool file_registry_service_updated)
            : file(std::move(file)), src(std::move(src)), dst(std::move(dst)),
              file_registry_service(std::move(file_registry_service)),
              file_registry_service_updated(file_registry_service_updated) {}

    public:
        /** @brief The workflow file that has successfully been copied */
        std::shared_ptr<DataFile> file;
        /** @brief The source location */
        std::shared_ptr<FileLocation> src;
        /** @brief The destination location */
        std::shared_ptr<FileLocation> dst;
        /** @brief The file registry service that was supposed to be updated (or nullptr if none) */
        std::shared_ptr<FileRegistryService> file_registry_service;
        /** @brief Whether the file registry service (if any) has been successfully updated */
        bool file_registry_service_updated;

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


};// namespace wrench

    /***********************/
    /** \endcond           */
    /***********************/


#endif//WRENCH_FILE_COPY_COMPLETED_EVENT_H
