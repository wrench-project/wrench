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
     * @param answer_commport: the commport to which to send the answer
     * @param file: the file for which storage allocation is requested
     * @param stripe_count: the strip count
     * @param payload: the message size in bytes
     *
     */
    CompoundStorageAllocationRequestMessage::CompoundStorageAllocationRequestMessage(S4U_CommPort *answer_commport,
                                                                                     std::shared_ptr<DataFile> file,
                                                                                     unsigned int stripe_count,
                                                                                     double payload)
        : CompoundStorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_commport == nullptr) {
            throw std::invalid_argument(
                    "CompoundStorageStorageSelectionRequestMessage::CompoundStorageStorageSelectionRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->file = std::move(file);
        this->stripe_count = stripe_count;
    }

    /**
     * @brief Constructor
     * @param locations: Existing or newly allocated FileLocations for requested file
     * @param payload: the message size in bytes
     *
     */
    CompoundStorageAllocationAnswerMessage::CompoundStorageAllocationAnswerMessage(
            std::vector<std::shared_ptr<FileLocation>> locations, double payload)
        : CompoundStorageServiceMessage(payload) {
        this->locations = std::move(locations);
    }

    /**
     * @brief Constructor
     * @param answer_commport: the commport to which to send the answer
     * @param file: the file for which storage allocation is requested
     * @param payload: the message size in bytes
     *
     */
    CompoundStorageLookupRequestMessage::CompoundStorageLookupRequestMessage(S4U_CommPort *answer_commport,
                                                                             std::shared_ptr<DataFile> file, double payload)
        : CompoundStorageServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_commport == nullptr) {
            throw std::invalid_argument(
                    "CompoundStorageStorageSelectionRequestMessage::CompoundStorageStorageSelectionRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->file = std::move(file);
    }

    /**
     * @brief Constructor
     * @param locations: Known FileLocations for requested file
     * @param payload: the message size in bytes
     *
     */
    CompoundStorageLookupAnswerMessage::CompoundStorageLookupAnswerMessage(
            std::vector<std::shared_ptr<FileLocation>> locations, double payload)
        : CompoundStorageServiceMessage(payload) {
        this->locations = std::move(locations);
    }

}// namespace wrench
