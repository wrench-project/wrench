/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPOUNDSTORAGESERVICE_H
#define WRENCH_COMPOUNDSTORAGESERVICE_H

#include "wrench/services/memory/MemoryManager.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/compound/CompoundStorageServiceMessage.h"
#include "wrench/services/storage/compound/CompoundStorageServiceMessagePayload.h"
#include "wrench/services/storage/compound/CompoundStorageServiceProperty.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"

namespace wrench {

    /**
     * @brief Specification of a callback
     *
     * @param file File/file part/chunk of data which needs to be allocated
     * @param resources Map of storage node hostnames and associated StorageServices (one per disk)
     * @param mapping Mapping of current allocations (files and the file parts locations)
     * @param previous_allocations Previous allocations of parts of the same file as the current file part (when the allocator doesn't do striping internally)
     * @return Vector of FileLocation (potentially with one element only) which should be used for the given file.
     */
    using StorageSelectionStrategyCallback = std::function<std::vector<std::shared_ptr<FileLocation>>(
            const std::shared_ptr<DataFile> &file,
            const std::map<std::string, std::vector<std::shared_ptr<StorageService>>> &resources,
            const std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> &mapping,
            const std::vector<std::shared_ptr<FileLocation>> &previous_allocations,
            unsigned int stripe_count)>;

    /**
     * @brief Enum for IO actions in traces
     */
    enum class IOAction : std::uint8_t {
        ReadStart = 1,
        ReadEnd = 2,
        WriteStart = 3,
        WriteEnd = 4,
        CopyToStart = 5,
        CopyToEnd = 6,
        CopyFromStart = 7,
        CopyFromEnd = 8,
        DeleteStart = 9,
        DeleteEnd = 10,
        None = 11,
    };

    /**
     * @brief Structure to track disk usage
     */
    struct DiskUsage {
        /** @brief Storage service */
        std::shared_ptr<StorageService> service;
        /** @brief Free space in byte */
        double free_space;
        uint64_t file_count;
    };

    /**
     * @brief Structure for tracing file allocations for each job
     */
    struct AllocationTrace {
        /** @brief time stamp */
        double ts;
        /** @brief IO action */
        IOAction act;
        int parts_count;// number of file parts in location array
        std::string file_name;
        std::vector<DiskUsage> disk_usage;// new usage stats for updated disks
        std::vector<std::shared_ptr<FileLocation>> internal_locations;
    };

    /**
     * @brief An abstract storage service which holds a collection of concrete storage services (eg.
     *        SimpleStorageServices). It does not provide direct access to any storage resource.
     *        It is meant to be used as a way to postpone the selection of a storage service for a file
     *        action (read, write, copy, etc) until a later time in the simulation, rather than during
     *        job definition. A typical use for the CompoundStorageService is to select a definitive
     *        SimpleStorageService for each action of a job during its scheduling in a BatchScheduler class.
     *        This should never receive messages for I/O operations, as any standard storage service
     *        (File Read/Write/Delete/Copy/Lookup requests), instead, it overides the main functions of
     *        StorageService (readFile / writeFile /...) and will craft messages intended for one or many of
     *        its underlying storage services.
     */
    class CompoundStorageService : public StorageService {
    public:
        using StorageService::createFile;
        using StorageService::deleteFile;
        using StorageService::hasFile;
        using StorageService::lookupFile;
        using StorageService::readFile;
        using StorageService::writeFile;

        CompoundStorageService(const std::string &hostname,
                               std::set<std::shared_ptr<StorageService>> storage_services,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        CompoundStorageService(const std::string &hostname,
                               std::set<std::shared_ptr<StorageService>> storage_services,
                               StorageSelectionStrategyCallback &allocate,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) override;

        double getLoad() override;

        // Overload StorageService's implementation.
        double getTotalSpace() override;

        // Overload StorageService's implementation.
        double getTotalFreeSpaceAtPath(const std::string &path) override;

        // Overload StorageService's implementation.
        void setIsScratch(bool is_scratch) override;

        /**
         * @brief Determine whether the storage service is bufferized
         * @return true if bufferized, false otherwise
         */
        bool isBufferized() const override {
            return false;
        }

        /**
         * @brief Determine the storage service's buffer size
         * @return a size in bytes
         */
        double getBufferSize() const override {
            return 0;
        }

        /**
         * @brief Reserve space at the storage service
         * @param location a location
         * @return true if success, false otherwise
         */
        bool reserveSpace(std::shared_ptr<FileLocation> &location) override {
            throw std::runtime_error("CompoundStorageService::reserveSpace(): not implemented");
        }

        /**
         * @brief Remove a directory and all its content at the storage service (in zero simulated time)
         * @param path: a path
         */
        void removeDirectory(const std::string &path) override {
            throw std::runtime_error("CompoundStorageService::removeDirectory(): not implemented");
        }

        /**
         * @brief Unreserve space at the storage service
         * @param location a location
         */
        void unreserveSpace(std::shared_ptr<FileLocation> &location) override {
            throw std::runtime_error("CompoundStorageService::unreserveSpace(): not implemented");
        }

        /**
         * @brief Create a file at the storage service (in zero simulated time)
         * @param location a location
         */
        void createFile(const std::shared_ptr<FileLocation> &location) override {
            throw std::runtime_error("CompoundStorageService::createFile(): not implemented");
        }

        /**
         * @brief Remove a file at the storage service (in zero simulated time)
         * @param location a location
         */
        void removeFile(const std::shared_ptr<FileLocation> &location) override {
            throw std::runtime_error("CompoundStorageService::removeFile(): not implemented");
        }

        /**
         * @brief Method to return the collection of known StorageServices
         */
        std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &getAllServices();

        std::vector<std::shared_ptr<FileLocation>> lookupFileLocation(const std::shared_ptr<FileLocation> &location);

        std::vector<std::shared_ptr<FileLocation>> lookupFileLocation(const std::shared_ptr<DataFile> &file, S4U_CommPort *answer_commport);

        std::vector<std::shared_ptr<FileLocation>> lookupOrDesignateStorageService(const std::shared_ptr<FileLocation> location);

        std::vector<std::shared_ptr<FileLocation>> lookupOrDesignateStorageService(const std::shared_ptr<FileLocation> location, unsigned int stripe_count);

        bool hasFile(const std::shared_ptr<FileLocation> &location) override;

        void writeFile(S4U_CommPort *answer_commport,
                       const std::shared_ptr<FileLocation> &location,
                       double num_bytes_to_write,
                       bool wait_for_answer) override;

        void readFile(S4U_CommPort *answer_commport,
                      const std::shared_ptr<FileLocation> &location,
                      double num_bytes,
                      bool wait_for_answer) override;

        void deleteFile(S4U_CommPort *answer_commport,
                        const std::shared_ptr<FileLocation> &location,
                        bool wait_for_answer) override;

        bool lookupFile(S4U_CommPort *answer_commport,
                        const std::shared_ptr<FileLocation> &location) override;

        /**
         * @brief Intended to be called by StorageService::copyFile() when the use
         *        of a CSS is detected in a file copy.
         */
        static void copyFile(const std::shared_ptr<FileLocation> &src_location,
                             const std::shared_ptr<FileLocation> &dst_location);

        void copyFileIamSource(const std::shared_ptr<FileLocation> &src_location,
                               const std::shared_ptr<FileLocation> &dst_location);

        void copyFileIamDestination(const std::shared_ptr<FileLocation> &src_location,
                                    const std::shared_ptr<FileLocation> &dst_location);

        // Publicly accessible traces... (TODO: cleanup access to traces)
        /** @brief File read traces */
        // std::map<std::string, AllocationTrace> read_traces = {};
        /** @brief File write traces */
        std::map<std::string, AllocationTrace> write_traces = {};
        /** @brief File copy traces */
        std::map<std::string, AllocationTrace> copy_traces = {};
        /** @brief File delete traces */
        std::map<std::string, AllocationTrace> delete_traces = {};
        /** @brief Internal storage use */
        std::vector<std::pair<double, AllocationTrace>> internal_storage_use = {};

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        /** @brief Default property values **/
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "64000000"},
                {CompoundStorageServiceProperty::INTERNAL_STRIPING, "true"},
        };

        /** @brief Default message payload values
         *         Some values are set to zero because in the current implementation, it is expected
         *         that the CompoundStorageService will always immediately refuse / reject such
         *         requests, with minimum cost to the user.
         */
        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {CompoundStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {CompoundStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {CompoundStorageServiceMessagePayload::FREE_SPACE_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD, 0},
                {CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD, S4U_CommPort::default_control_message_size}};

        static unsigned long getNewUniqueNumber();

        bool processStopDaemonRequest(S4U_CommPort *ack_commport);

        /***********************/
        /** \endcond           */
        /***********************/
    private:
        friend class Simulation;

        int main() override;

        std::vector<std::shared_ptr<FileLocation>> lookupOrDesignateStorageService(const std::shared_ptr<DataFile> concrete_file_location,
                                                                                   unsigned int stripe_count,
                                                                                   S4U_CommPort *answer_commport);

        bool processStorageSelectionMessage(const CompoundStorageAllocationRequestMessage *msg);

        bool processStorageLookupMessage(const CompoundStorageLookupRequestMessage *msg);

        bool processNextMessage(SimulationMessage *message);

        /* Key : hostname of storage server, value : list of storage services (one per disk / raid / ...) on this storage server */
        std::map<std::string, std::vector<std::shared_ptr<StorageService>>> storage_services = {};

        unsigned int total_nb_storage_services = 0;

        std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> file_location_mapping = {};

        std::map<std::shared_ptr<DataFile>, unsigned int> partial_io_stripe_index;

        StorageSelectionStrategyCallback &allocate;

        /**
         * @brief Chunk size for file stripping
         *        Should usually be user-provided, or will be
         *        set to the smallest disk size as default.
         */
        double max_chunk_size = 0;

        /**
         *  @brief  Whether to strip a file in the CSS or in the external allocation function.
         *          Internal flag set from CSS property.
         */
        bool internal_stripping;

        /**
         * @brief Dirty log tracing method (needs to be improved)
         */
        void traceInternalStorageUse(IOAction action, const std::vector<std::shared_ptr<FileLocation>> &locations = {});
    };

};// namespace wrench

#endif// WRENCH_COMPOUNDSTORAGESERVICE_H
