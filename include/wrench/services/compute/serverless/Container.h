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
#include <fsmod/File.hpp>
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

    public:
        enum class State {
            BUSY,
            IDLE,
        };
        [[nodiscard]] bool isIdle() const { return _state == State::IDLE; }
        [[nodiscard]] bool isBusy() const { return _state == State::BUSY; }
        // [[nodiscard]] double getIdleTime() const;
        [[nodiscard]] std::uint64_t getIdleSequence() const {return _idle_sequence; }
        [[nodiscard]] const RegisteredFunction *getRegisteredFunction() const { return _registered_function; }
        [[nodiscard]] std::shared_ptr<StorageService> getPrivateStorageService() const { return _tmp_storage_service; }
        [[nodiscard]] ServerlessComputeNode* getComputeNode() const {return const_cast<ServerlessComputeNode*>(_compute_node); };

    private:
        friend class ServerlessComputeNode;

        explicit Container(const RegisteredFunction* registered_function,
                           const ServerlessComputeNode* compute_node,
                           const ServerlessComputeService* serverless_compute_service,
                           State initial_state) {
            _registered_function = registered_function;
            _compute_node = compute_node;
            _serverless_compute_service = serverless_compute_service;
            _state = initial_state;
            _idle_date = -1.0;
        }

        void makeIdle();
        void makeBusy();
        void spawn();
        void shutdown();

        void freeDiskAndMemoryResources();

        const RegisteredFunction* _registered_function;
        const ServerlessComputeNode *_compute_node;
        const ServerlessComputeService* _serverless_compute_service;
        State _state;
        double _idle_date;

        std::shared_ptr<FileLocation> _tmp_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_file;
        std::shared_ptr<StorageService> _tmp_storage_service;

        std::shared_ptr<simgrid::fsmod::File> _opened_image_ram_file;

        std::shared_ptr<FileLocation> _tmp_ram_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_ram_file;

        unsigned long _idle_sequence = 0;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif //CONTAINER_H
