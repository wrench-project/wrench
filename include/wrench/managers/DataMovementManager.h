/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_DATAMOVEMENTMANAGER_H
#define WRENCH_DATAMOVEMENTMANAGER_H


#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class Workflow;
    class WorkflowFile;
    class StorageService;
    class ExecutionController;
    class WMS;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A helper daemon (co-located with an execution controler) that handles data movement operations
     */
    class DataMovementManager : public Service {

    public:

        void stop() override;

        void kill();

        void initiateAsynchronousFileCopy(WorkflowFile *file,
                                          std::shared_ptr<FileLocation> src,
                                          std::shared_ptr<FileLocation> dst,
                                          std::shared_ptr<FileRegistryService> file_registry_service=nullptr);

        void doSynchronousFileCopy(WorkflowFile *file,
                                   std::shared_ptr<FileLocation> src,
                                   std::shared_ptr<FileLocation> dst,
                                   std::shared_ptr<FileRegistryService> file_registry_service=nullptr);


    protected:

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        friend class WMS;
        friend class ExecutionController;

        explicit DataMovementManager(std::string hostname, std::string &creator_mailbox);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        int main() override;

        std::string &getCreatorMailbox();

        std::string creator_mailbox;

        bool processNextMessage();

        struct CopyRequestSpecs {
            WorkflowFile *file;
            std::shared_ptr<FileLocation> src;
            std::shared_ptr<FileLocation> dst;
            std::shared_ptr<FileRegistryService> file_registry_service;

            CopyRequestSpecs(WorkflowFile *file,
                             std::shared_ptr<FileLocation> src,
                             std::shared_ptr<FileLocation> dst,
                             std::shared_ptr<FileRegistryService> file_registry_service) :
                    file(file), src(src), dst(dst), file_registry_service(file_registry_service) {}

            bool operator==(const CopyRequestSpecs &rhs) const {
                return (file == rhs.file) &&
                       (dst->getStorageService() == rhs.dst->getStorageService())  &&
                       (dst->getAbsolutePathAtMountPoint()     == rhs.dst->getAbsolutePathAtMountPoint());
            }
        };

        std::list<std::unique_ptr<CopyRequestSpecs>> pending_file_copies;
    };

    /***********************/
    /** \endcond            */
    /***********************/


};


#endif //WRENCH_DATAMOVEMENTMANAGER_H
