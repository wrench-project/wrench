/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/file_registry/FileRegistryMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    FileRegistryMessage::FileRegistryMessage(double payload) : ServiceMessage(payload) {
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file to look up
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupRequestMessage::FileRegistryFileLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                               const std::shared_ptr<DataFile> &file, double payload) : FileRegistryMessage(payload), answer_mailbox(answer_mailbox), file(file) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || file == nullptr) {
            throw std::invalid_argument(
                    "FileRegistryFileLookupRequestMessage::FileRegistryFileLookupRequestMessage(): Invalid argument");
        }
#endif
    }

    /**
     * @brief Constructor
     * @param locations: the set of locations for the file
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupAnswerMessage::FileRegistryFileLookupAnswerMessage(std::set<std::shared_ptr<FileLocation>> locations,
                                                                             double payload) : FileRegistryMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
//        if (locations.empty()) {
//            throw std::invalid_argument(
//                    "FileRegistryFileLookupAnswerMessage::FileRegistryFileLookupAnswerMessage(): Invalid argument");
//        }
#endif
        this->locations = std::move(locations);
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file to look up
     * @param reference_host: the host from which network proximity will be calculated from //
     * @param network_proximity_service: a reference to the network proximity service to be used
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupByProximityRequestMessage::FileRegistryFileLookupByProximityRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<DataFile> file, std::string reference_host,
            std::shared_ptr<NetworkProximityService> network_proximity_service, double payload) : FileRegistryMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (answer_mailbox == nullptr) || (reference_host == "") ||
            (network_proximity_service == nullptr)) {
            throw std::invalid_argument(
                    "FileRegistryFileLookupByProximityRequestMessage::FileRegistryFileLookupByProximityRequestMessage(): Invalid Argument");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = std::move(file);
        this->reference_host = std::move(reference_host);
        this->network_proximity_service = std::move(network_proximity_service);
    }

    /**
     * @brief Constructor
     * @param file: the file to look up
     * @param reference_host: the host from which network proximity will be calculated from // 
     * @param locations: the map of locations at which the file resides in ascending order with respect to their distance (network proximity) from 'reference_host'
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupByProximityAnswerMessage::FileRegistryFileLookupByProximityAnswerMessage(
            std::shared_ptr<DataFile> file, std::string reference_host,
            std::map<double, std::shared_ptr<FileLocation>> locations,
            double payload) : FileRegistryMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((file == nullptr) || (reference_host.empty())) {
            throw std::invalid_argument(
                    "FileRegistryFileLookupByProximityAnswerMessage::FileRegistryFileLookupByProximityAnswerMessage(): Invalid Argument");
        }
#endif
        this->file = std::move(file);
        this->reference_host = std::move(reference_host);
        this->locations = std::move(locations);
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param location: the file location of that entry
     * @param payload: the message size in bytes
     */
    FileRegistryRemoveEntryRequestMessage::FileRegistryRemoveEntryRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                 std::shared_ptr<FileLocation> location,
                                                                                 double payload) : FileRegistryMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "FileRegistryRemoveEntryRequestMessage::FileRegistryRemoveEntryRequestMessage(): Invalid argument");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->location = std::move(location);
    }


    /**
     * @brief Constructor
     * @param success: whether the entry removal was successful
     * @param payload: the message size in bytes
     */
    FileRegistryRemoveEntryAnswerMessage::FileRegistryRemoveEntryAnswerMessage(bool success,
                                                                               double payload) : FileRegistryMessage(payload) {
        this->success = success;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param location: the location for the new entry
     * @param payload: the message size in bytes
     */
    FileRegistryAddEntryRequestMessage::FileRegistryAddEntryRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                           std::shared_ptr<FileLocation> location,
                                                                           double payload) : FileRegistryMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (location == nullptr)) {
            throw std::invalid_argument(
                    "FileRegistryAddEntryRequestMessage::FileRegistryAddEntryRequestMessage(): Invalid argument");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->location = std::move(location);
    }

    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    FileRegistryAddEntryAnswerMessage::FileRegistryAddEntryAnswerMessage(double payload) : FileRegistryMessage(payload) {
    }

}// namespace wrench
