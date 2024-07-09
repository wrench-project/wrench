/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <algorithm>
#include <utility>

#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/services/ServiceMessage.h>

#include <wrench/services/file_registry/FileRegistryService.h>
#include "wrench/services/file_registry/FileRegistryMessage.h"
#include <wrench/services/storage/StorageService.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/services/network_proximity/NetworkProximityService.h>
#include <wrench/failure_causes/NetworkError.h>

WRENCH_LOG_CATEGORY(wrench_core_file_registry_service,
                    "Log category for File Registry Service");

namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    FileRegistryService::FileRegistryService(
            std::string hostname,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : Service(std::move(hostname), "file_registry") {
        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }

    FileRegistryService::~FileRegistryService() {
    }

    /**
     * @brief Lookup entries for a file
     * @param file: the file to lookup
     * @return The list locations for the file
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    std::set<std::shared_ptr<FileLocation>> FileRegistryService::lookupEntry(const std::shared_ptr<DataFile> &file) {
        if (file == nullptr) {
            throw std::invalid_argument("FileRegistryService::lookupEntry(): Invalid argument");
        }

        assertServiceIsUp();

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        this->commport->putMessage(new FileRegistryFileLookupRequestMessage(
                answer_commport, file,
                this->getMessagePayloadValue(
                        FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

        auto msg = answer_commport->getMessage<FileRegistryFileLookupAnswerMessage>(
                this->network_timeout,
                "FileRegistryService::lookupEntry(): Received in");
        return msg->locations;
    }

    /**
     * @brief Lookup entries for a file, including for each entry a network distance from a reference host (as
     *        determined by a network proximity service)
     * @param file: the file to lookup
     * @param reference_host: reference host from which network proximity values are to be measured
     * @param network_proximity_service: the network proximity service to use
     *
     * @return a map of <distance , file location> pairs
     */
    std::map<double, std::shared_ptr<FileLocation>> FileRegistryService::lookupEntry(
            const std::shared_ptr<DataFile> &file,
            const std::string &reference_host,
            const std::shared_ptr<NetworkProximityService> &network_proximity_service) {
        if (file == nullptr) {
            throw std::invalid_argument("FileRegistryService::lookupEntryByProximity(): Invalid argument, no file");
        }

        assertServiceIsUp();

        // check to see if the 'reference_host' is valid
        std::vector<std::string> monitored_hosts = network_proximity_service->getHostnameList();
        if (std::find(monitored_hosts.cbegin(), monitored_hosts.cend(), reference_host) == monitored_hosts.cend()) {
            throw std::invalid_argument(
                    "FileRegistryService::lookupEntryByProximity(): Invalid argument, host " + reference_host +
                    " does not exist");
        }

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        this->commport->putMessage(
                new FileRegistryFileLookupByProximityRequestMessage(
                        answer_commport, file,
                        reference_host,
                        network_proximity_service,
                        this->getMessagePayloadValue(
                                FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

        auto msg = answer_commport->getMessage<FileRegistryFileLookupByProximityAnswerMessage>(
                this->network_timeout,
                "FileRegistryService::lookupEntry(): Received an");

        return msg->locations;
    }

    /**
     * @brief Add an entry
     * @param location: a file location
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void FileRegistryService::addEntry(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("FileRegistryService::addEntry(): Invalid nullptr argument");
        }
        assertServiceIsUp();

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        this->commport->putMessage(
                new FileRegistryAddEntryRequestMessage(
                        answer_commport, location,
                        this->getMessagePayloadValue(
                                FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD)));

        auto msg = answer_commport->getMessage<FileRegistryAddEntryAnswerMessage>(
                this->network_timeout,
                "FileRegistryService::addEntry(): Received an");
    }

    /**
     * @brief Remove an entry
     * @param location: a file location
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void FileRegistryService::removeEntry(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument(" FileRegistryService::removeEntry(): Invalid nullptr argument");
        }

        assertServiceIsUp();

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        this->commport->putMessage(
                new FileRegistryRemoveEntryRequestMessage(
                        answer_commport, location,
                        this->getMessagePayloadValue(
                                FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD)));

        auto msg = answer_commport->getMessage<FileRegistryRemoveEntryAnswerMessage>(
                this->network_timeout,
                "FileRegistryService::removeEntry(): Received an");
        if (!msg->success) {
            WRENCH_WARN(
                    "Attempted to remove non-existent (%s,%s) entry from file registry service (ignored)",
                    location->getFile()->getID().c_str(), location->toString().c_str());
        }
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int FileRegistryService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("File Registry Service starting on host %s!", S4U_Simulation::getHostName().c_str());

        /** Main loop **/
        while (this->processNextMessage()) {
        }

        WRENCH_INFO("File Registry Service on host %s cleanly terminating!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Helper function to process incoming messages
     * @return false if the daemon should terminate after processing this message
     */
    bool FileRegistryService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // This is Synchronous
            try {
                msg->ack_commport->putMessage(
                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                FileRegistryServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<FileRegistryFileLookupRequestMessage>(message)) {
            std::set<std::shared_ptr<FileLocation>> locations = {};
            if (this->entries.find(msg->file) != this->entries.end()) {
                locations = this->entries[msg->file];
            }
            // Simulate a lookup overhead
            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::LOOKUP_COMPUTE_COST));

            msg->answer_commport->dputMessage(
                    new FileRegistryFileLookupAnswerMessage(
                            locations,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<FileRegistryFileLookupByProximityRequestMessage>(message)) {
            std::string reference_host = msg->reference_host;

            std::set<std::shared_ptr<FileLocation>> all_file_locations = {};
            if (this->entries.find(msg->file) != this->entries.end()) {
                all_file_locations = this->entries[msg->file];
            }

            double proximity;
            std::map<double, std::shared_ptr<FileLocation>> map_to_return;
            auto itr = map_to_return.cbegin();

            for (auto &location: all_file_locations) {
                proximity = std::get<0>(msg->network_proximity_service->getHostPairDistance(
                        std::make_pair(reference_host, location->getStorageService()->getHostname())));
                itr = map_to_return.insert(itr, std::make_pair(proximity, location));
            }

            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::LOOKUP_COMPUTE_COST));
            msg->answer_commport->dputMessage(
                    new FileRegistryFileLookupByProximityAnswerMessage(
                            msg->file,
                            msg->reference_host,
                            map_to_return,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<FileRegistryAddEntryRequestMessage>(message)) {
            addEntryToDatabase(msg->location);

            // Simulate an add overhead
            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::ADD_ENTRY_COMPUTE_COST));

            msg->answer_commport->dputMessage(
                    new FileRegistryAddEntryAnswerMessage(this->getMessagePayloadValue(
                            FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<FileRegistryRemoveEntryRequestMessage>(message)) {
            bool success = removeEntryFromDatabase(msg->location);

            // Simulate a removal overhead
            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::REMOVE_ENTRY_COMPUTE_COST));

            msg->answer_commport->dputMessage(
                    new FileRegistryRemoveEntryAnswerMessage(
                            success,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::REMOVE_ENTRY_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else {
            throw std::runtime_error(
                    "FileRegistryService::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * Internal method to add an entry to the database
     * @param location: a file location
     */
    void FileRegistryService::addEntryToDatabase(const std::shared_ptr<FileLocation> &location) {
        auto file = location->getFile();
        WRENCH_INFO("Adding file (%s) at (%s) to the file registry", file->getID().c_str(),
                    location->getStorageService()->getHostname().c_str());
        if (this->entries.find(file) == this->entries.end()) {
            this->entries[file] = {};
        }
        for (const auto &e: this->entries[file]) {
            if (FileLocation::equal(e, location)) {
                return;
            }
        }
        this->entries[file].insert(location);
    }

    /**
     * Internal method to remove an entry from the database
     * @param location: a file location
     *
     * @return true if an entry was removed
     */
    bool FileRegistryService::removeEntryFromDatabase(const std::shared_ptr<FileLocation> &location) {
        auto file = location->getFile();
        WRENCH_INFO("Removing file (%s) from the file registry", file->getID().c_str());
        if (this->entries.find(file) != this->entries.end()) {
            for (auto const &l: this->entries[file]) {
                if (FileLocation::equal(l, location)) {
                    this->entries[file].erase(l);
                    return true;
                }
            }
        }
        return false;
    }

}// namespace wrench
