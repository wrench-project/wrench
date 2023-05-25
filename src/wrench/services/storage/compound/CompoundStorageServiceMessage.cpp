/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/storage/compound/CompoundStorageServiceMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     * @param payload: the message size in bytes
     */
    CompoundStorageServiceMessage::CompoundStorageServiceMessage(double payload) : StorageServiceMessage(payload) {}

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file for which storage allocation is requested
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    CompoundStorageAllocationRequestMessage::CompoundStorageAllocationRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                                     std::shared_ptr<DataFile> file, double payload)
        : CompoundStorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_mailbox == nullptr) {
            throw std::invalid_argument(
                    "CompoundStorageStorageSelectionRequestMessage::CompoundStorageStorageSelectionRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = file;
    }

    /**
     * @brief Constructor
     * @param locations: Existing or newly allocated FileLocations for requested file
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CompoundStorageAllocationAnswerMessage::CompoundStorageAllocationAnswerMessage(
            std::vector<std::shared_ptr<FileLocation>> locations, double payload)
        : CompoundStorageServiceMessage(payload) {
        this->locations = locations;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param file: the file for which storage allocation is requested
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    CompoundStorageLookupRequestMessage::CompoundStorageLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                                             std::shared_ptr<DataFile> file, double payload)
        : CompoundStorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_mailbox == nullptr) {
            throw std::invalid_argument(
                    "CompoundStorageStorageSelectionRequestMessage::CompoundStorageStorageSelectionRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->file = file;
    }

    /**
     * @brief Constructor
     * @param locations: Known FileLocations for requested file
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    CompoundStorageLookupAnswerMessage::CompoundStorageLookupAnswerMessage(
            std::vector<std::shared_ptr<FileLocation>> locations, double payload)
        : CompoundStorageServiceMessage(payload) {
        this->locations = locations;
    }

}// namespace wrench
