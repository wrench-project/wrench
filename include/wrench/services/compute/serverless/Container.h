/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef CONTAINER_H
#define CONTAINER_H

#include <memory>
#include <cfloat>
#include <fsmod/File.hpp>
#include <wrench/function/Function.h>

namespace wrench {
    class ServerlessComputeNode;
    class Invocation;
    class StorageService;

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @class Container
     * @brief Represents a container in a serverless platform.
     */
    class Container {

    public:
	/**
	 * @brief Enum to describe a container's state
	 */
        enum class State {
            BUSY,
            IDLE,
        };

        [[nodiscard]] unsigned long getCreationId() const;
        [[nodiscard]] bool isIdle() const;
        [[nodiscard]] bool isBusy() const;
        [[nodiscard]] unsigned long getIdleSequence() const;
        [[nodiscard]] double getIdleDate() const;
        [[nodiscard]] const Function *getFunction() const;
        [[nodiscard]] std::shared_ptr<StorageService> getPrivateStorageService() const;
        [[nodiscard]] ServerlessComputeNode* getComputeNode() const;

        void clearPrivateStorage() const;

    private:
        friend class ServerlessComputeNode;

        Container(const Function* function,
                           const ServerlessComputeNode* compute_node,
                           const ServerlessComputeService* serverless_compute_service,
                           State initial_state);

        void makeIdle();
        void makeBusy();
        void spawn();
        void shutdown();

        void freeDiskAndMemoryResources();

        const Function* _function;
        const ServerlessComputeNode *_compute_node;
        const ServerlessComputeService* _serverless_compute_service;
        State _state;

        std::shared_ptr<FileLocation> _tmp_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_file;
        std::shared_ptr<StorageService> _tmp_storage_service;

        std::set<std::shared_ptr<simgrid::fsmod::File>> _opened_image_layer_disk_files;
        std::set<std::shared_ptr<simgrid::fsmod::File>> _opened_image_layer_ram_files;

        std::shared_ptr<FileLocation> _tmp_ram_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_ram_file;

        static unsigned long _creation_id_counter;
        unsigned long _creation_id; // to break ties
        unsigned long _idle_sequence = 0;
        double _idle_date = DBL_MAX;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif //CONTAINER_H
