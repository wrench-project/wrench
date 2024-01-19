/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPOUNDSTORAGESERVICEMESSAGE_H
#define WRENCH_COMPOUNDSTORAGESERVICEMESSAGE_H

#include <memory>
#include <utility>

#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageServiceMessage.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a StorageService
     */
    class CompoundStorageServiceMessage : public StorageServiceMessage {
    protected:
        CompoundStorageServiceMessage(double payload);
    };

    /**
     * @brief A message sent to a CompoundStorageService to request a storage allocation for a file
     */
    class CompoundStorageAllocationRequestMessage : public CompoundStorageServiceMessage {
    public:
        CompoundStorageAllocationRequestMessage(S4U_CommPort *answer_commport, std::shared_ptr<DataFile> file, double payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The path */
        std::shared_ptr<DataFile> file;
    };

    /**
     * @brief A message sent by a StorageService in answer to a storage selection request
     */
    class CompoundStorageAllocationAnswerMessage : public CompoundStorageServiceMessage {
    public:
        CompoundStorageAllocationAnswerMessage(std::vector<std::shared_ptr<FileLocation>> locations, double payload);

        /** @brief Known or newly allocated FileLocations for requested file */
        std::vector<std::shared_ptr<FileLocation>> locations;
    };

    /**
     * @brief A message sent to a CompoundStorageService to request a storage allocation for a file
     */
    class CompoundStorageLookupRequestMessage : public CompoundStorageServiceMessage {
    public:
        CompoundStorageLookupRequestMessage(S4U_CommPort *answer_commport, std::shared_ptr<DataFile> file, double payload);

        /** @brief CommPort to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The path */
        std::shared_ptr<DataFile> file;
    };

    /**
     * @brief A message sent by a StorageService in answer to a storage selection request
     */
    class CompoundStorageLookupAnswerMessage : public CompoundStorageServiceMessage {
    public:
        CompoundStorageLookupAnswerMessage(std::vector<std::shared_ptr<FileLocation>> locations, double payload);

        /** @brief Known FileLocations for requested file */
        std::vector<std::shared_ptr<FileLocation>> locations;
    };

    /***********************/
    /** \endcond INTERNAL     */
    /***********************/
}// namespace wrench

#endif// WRENCH_COMPOUNDSTORAGESERVICEMESSAGE_H
