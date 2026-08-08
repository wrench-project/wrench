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
#include <wrench/managers/function_manager/RegisteredFunction.h>

namespace wrench {
    class ServerlessComputeNode;

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @class Container
     * @brief Represents a container in a serverless platform.
     */
    class Container {

      enum class ContainerState {
            BUSY,
            IDLE,
        };

    public:

        explicit Container(const std::shared_ptr<RegisteredFunction> &registered_function,
            const std::shared_ptr<ServerlessComputeNode> &compute_node,
            ServerlessComputeService* serverless_compute_service) {
            _registered_function = registered_function;
            _compute_node = compute_node;
            _serverless_compute_service = serverless_compute_service;
            _state = ContainerState::BUSY;

         }

        [[nodiscard]] bool isIdle() const { return _state == ContainerState::IDLE; }
        void makeIdle() { _state = ContainerState::IDLE; }
        void makeBusy() { _state = ContainerState::BUSY; }

        void spawn();
        void shutdown();

        std::shared_ptr<StorageService> getPrivateStorageService() const {return _tmp_storage_service; }


    private:
        std::shared_ptr<RegisteredFunction> _registered_function;
        std::shared_ptr<ServerlessComputeNode> _compute_node;
        ServerlessComputeService *_serverless_compute_service;
        ContainerState _state;

        std::shared_ptr<FileLocation> _tmp_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_file;
        std::shared_ptr<StorageService> _tmp_storage_service;

        std::shared_ptr<simgrid::fsmod::File> _opened_image_ram_file;

        std::shared_ptr<FileLocation> _tmp_ram_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_ram_file;

    /***********************/
    /** \endcond          **/
    /***********************/

    };
}

#endif //CONTAINER_H
