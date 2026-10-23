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
    class Alarm;
    class ServerlessComputeNode;
    class Invocation;
    class StorageService;
    class FailureCause;

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
            INITIALIZING,
            INITIALIZED,
        };

        [[nodiscard]] unsigned long getCreationId() const;
        [[nodiscard]] unsigned long getNumInflightInvocations() const;
        [[nodiscard]] unsigned long getMaxNumInflightInvocations() const;
        [[nodiscard]] bool isInitializing() const;
        [[nodiscard]] bool isInitialized() const;
        [[nodiscard]] bool isAtInvocationLimit() const;
        [[nodiscard]] unsigned long getIdleSequence() const;
        [[nodiscard]] double getIdleDate() const;
        [[nodiscard]] double getExpirationDate() const;
        [[nodiscard]] const Function *getFunction() const;
        [[nodiscard]] std::shared_ptr<StorageService> getPrivateStorageService() const;
        [[nodiscard]] ServerlessComputeNode* getComputeNode() const;

        void clearPrivateStorage() const;

    private:
        friend class ServerlessComputeNode;
        friend class ServerlessComputeService;

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
        std::shared_ptr<FailureCause> _init_failure_cause;

        std::set<std::shared_ptr<Invocation>> _in_flight_invocations;

        std::shared_ptr<FileLocation> _tmp_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_file;
        std::shared_ptr<StorageService> _tmp_storage_service;

        std::set<std::shared_ptr<simgrid::fsmod::File>> _opened_image_layer_disk_files;
        std::set<std::shared_ptr<simgrid::fsmod::File>> _opened_image_layer_ram_files;

        std::shared_ptr<FileLocation> _tmp_ram_file_location;
        std::shared_ptr<simgrid::fsmod::File> _opened_tmp_ram_file;

        unsigned long _max_num_in_flight_invocations;

        static unsigned long _creation_id_counter;
        unsigned long _creation_id; // to break ties
        unsigned long _idle_sequence = 0;
        double _idle_date = DBL_MAX;

        bool _initialized = false;

        double _expiration_date = DBL_MAX;
        std::shared_ptr<Alarm> _expiration_alarm = nullptr;

        /***********************/
        /** \endcond          **/
        /***********************/
    };
}

#endif //CONTAINER_H
