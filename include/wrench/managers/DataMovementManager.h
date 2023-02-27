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

#include <list>
#include <utility>

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    class Workflow;
    class DataFile;
    class StorageService;
    class FileRegistryService;
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

        void initiateAsynchronousFileCopy(const std::shared_ptr<FileLocation> &src,
                                          const std::shared_ptr<FileLocation> &dst,
                                          const std::shared_ptr<FileRegistryService> &file_registry_service = nullptr);

        void doSynchronousFileCopy(const std::shared_ptr<FileLocation> &src,
                                   const std::shared_ptr<FileLocation> &dst,
                                   const std::shared_ptr<FileRegistryService> &file_registry_service = nullptr);


    protected:
        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        friend class WMS;
        friend class ExecutionController;

        explicit DataMovementManager(std::string hostname, simgrid::s4u::Mailbox *creator_mailbox);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        int main() override;

        //        simgrid::s4u::Mailbox *getCreatorMailbox();

        simgrid::s4u::Mailbox *creator_mailbox;

        bool processNextMessage();

        struct CopyRequestSpecs {
            std::shared_ptr<FileLocation> src;
            std::shared_ptr<FileLocation> dst;
            std::shared_ptr<FileRegistryService> file_registry_service;

            ~CopyRequestSpecs() {
            }

            CopyRequestSpecs(std::shared_ptr<FileLocation> src,
                             std::shared_ptr<FileLocation> dst,
                             std::shared_ptr<FileRegistryService> file_registry_service) : src(std::move(src)), dst(std::move(dst)), file_registry_service(std::move(file_registry_service)) {}

            bool operator==(const CopyRequestSpecs &rhs) const {
                return (dst->getFile() == rhs.dst->getFile()) &&
                       (dst->getStorageService() == rhs.dst->getStorageService()) &&
                       (dst->getPath() == rhs.dst->getPath());
            }
        };

        std::list<std::unique_ptr<CopyRequestSpecs>> pending_file_copies;
    };

    /***********************/
    /** \endcond            */
    /***********************/


}// namespace wrench


#endif//WRENCH_DATAMOVEMENTMANAGER_H
