/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <algorithm>

#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/services/ServiceMessage.h>

#include "wrench/services/file_registry/FileRegistryService.h"
#include "FileRegistryMessage.h"
#include <wrench/services/storage/StorageService.h>
#include <wrench/workflow/WorkflowFile.h>
#include <wrench/exceptions/WorkflowExecutionException.h>
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
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list) :
            Service(hostname, "file_registry", "file_registry") {

        this->setProperties(this->default_property_values, property_list);
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    FileRegistryService::~FileRegistryService() {
        this->default_property_values.clear();
    }

    /**
     * @brief Lookup entries for a file
     * @param file: the file to lookup
     * @return The list locations for the file
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    std::set<std::shared_ptr<FileLocation>> FileRegistryService::lookupEntry(WorkflowFile *file) {
        if (file == nullptr) {
            throw std::invalid_argument("FileRegistryService::lookupEntry(): Invalid argument");
        }

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("lookup_entry");

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new FileRegistryFileLookupRequestMessage(
                    answer_mailbox, file,
                    this->getMessagePayloadValue(
                            FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {

            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<FileRegistryFileLookupAnswerMessage*>(message.get())) {
            std::set<std::shared_ptr<FileLocation>> result = msg->locations;
//        msg->locations.clear(); // TODO: Understand why this removes a memory_manager_service leak
            return result;
        } else {
            throw std::runtime_error(
                    "FileRegistryService::lookupEntry(): Unexpected [" + message->getName() + "] message");
        }
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
            WorkflowFile *file,
            std::string reference_host,
            std::shared_ptr<NetworkProximityService> network_proximity_service) {

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

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("lookup_entry_by_proximity");

        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new FileRegistryFileLookupByProximityRequestMessage(
                            answer_mailbox, file,
                            reference_host,
                            network_proximity_service,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<FileRegistryFileLookupByProximityAnswerMessage*>(message.get())) {
            return msg->locations;
        } else {
            throw std::runtime_error(
                    "FileRegistryService::lookupEntry(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Add an entry
     * @param file: a file
     * @param location: a file location
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void FileRegistryService::addEntry(WorkflowFile *file, std::shared_ptr<FileLocation> location) {
        if ((file == nullptr) || (location == nullptr)) {
            throw std::invalid_argument("FileRegistryService::addEntry(): Invalid  argument");
        }

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("add_entry");

        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new FileRegistryAddEntryRequestMessage(
                            answer_mailbox, file, location,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::ADD_ENTRY_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<FileRegistryAddEntryAnswerMessage*>(message.get())) {
            return;
        } else {
            std::runtime_error("FileRegistryService::addEntry(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Remove an entry
     * @param file: a file
     * @param location: a file location
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void FileRegistryService::removeEntry(WorkflowFile *file, std::shared_ptr<FileLocation> location) {
        if ((file == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(" FileRegistryService::removeEntry(): Invalid input argument");
        }

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("remove_entry");

        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new FileRegistryRemoveEntryRequestMessage(
                            answer_mailbox, file, location,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::REMOVE_ENTRY_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<FileRegistryRemoveEntryAnswerMessage*>(message.get())) {
            if (!msg->success) { WRENCH_WARN(
                        "Attempted to remove non-existent (%s,%s) entry from file registry service (ignored)",
                        file->getID().c_str(), location->toString().c_str());
            }
            return;
        } else {
            std::runtime_error("FileRegistryService::removeEntry(): Unexpected [" + message->getName() + "] message");
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
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage*>(message.get())) {
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                FileRegistryServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<FileRegistryFileLookupRequestMessage*>(message.get())) {
            std::set<std::shared_ptr<FileLocation>> locations = {};
            if (this->entries.find(msg->file) != this->entries.end()) {
                locations = this->entries[msg->file];
            }
            // Simulate a lookup overhead
            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::LOOKUP_COMPUTE_COST));

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
                    new FileRegistryFileLookupAnswerMessage(
                            msg->file, locations,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = dynamic_cast<FileRegistryFileLookupByProximityRequestMessage*>(message.get())) {
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
            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
                    new FileRegistryFileLookupByProximityAnswerMessage(
                            msg->file,
                            msg->reference_host,
                            map_to_return,
                            this->getMessagePayloadValue(
                                    FileRegistryServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = dynamic_cast<FileRegistryAddEntryRequestMessage*>(message.get())) {
            addEntryToDatabase(msg->file, msg->location);

            // Simulate an add overhead
            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::ADD_ENTRY_COMPUTE_COST));

            S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                     new FileRegistryAddEntryAnswerMessage(this->getMessagePayloadValue(
                                             FileRegistryServiceMessagePayload::ADD_ENTRY_ANSWER_MESSAGE_PAYLOAD)));
            return true;

        } else if (auto msg = dynamic_cast<FileRegistryRemoveEntryRequestMessage*>(message.get())) {
            bool success = removeEntryFromDatabase(msg->file, msg->location);

            // Simulate a removal overhead
            S4U_Simulation::compute(getPropertyValueAsDouble(FileRegistryServiceProperty::REMOVE_ENTRY_COMPUTE_COST));

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
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
     * @param file: a file
     * @param storage_service: a storage_service
     */
    void FileRegistryService::addEntryToDatabase(WorkflowFile *file, std::shared_ptr<FileLocation> location) {
        WRENCH_INFO("Adding file (%s) at (%s) to the file registry", file->getID().c_str(),
                    location->getStorageService()->getHostname().c_str());
        if (this->entries.find(file) == this->entries.end()) {
            this->entries[file] = {};
        }
        for (auto e : this->entries[file]) {
            if (FileLocation::equal(e, location)) {
                return;
            }
        }
        this->entries[file].insert(location);
    }

    /**
     * Internal method to remove an entry from the database
     * @param file: a file
     * @param location: a location
     *
     * @return true if an entry was removed
     */
    bool FileRegistryService::removeEntryFromDatabase(WorkflowFile *file, std::shared_ptr<FileLocation> location) {
        WRENCH_INFO("Removing file (%s) from the file registry", file->getID().c_str());
        if (this->entries.find(file) != this->entries.end()) {
            for (auto const &l : this->entries[file]) {
                if (FileLocation::equal(l, location)) {
                    this->entries[file].erase(l);
                    return true;

                }
            }
        }
        return false;
    }

};
