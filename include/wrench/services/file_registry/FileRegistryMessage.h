/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILEREGISTRYMESSAGE_H
#define WRENCH_FILEREGISTRYMESSAGE_H


#include <iostream>
#include "wrench/services/ServiceMessage.h"
#include "wrench/data_file/DataFile.h"
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include "wrench/services/storage/StorageService.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief Top-level FileRegistryMessage class
     */
    class FileRegistryMessage : public ServiceMessage {
    protected:
        FileRegistryMessage(sg_size_t payload);
    };

    /**
     * @brief A message sent to a FileRegistryService to request a file lookup
     */
    class FileRegistryFileLookupRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupRequestMessage(S4U_CommPort *answer_commport, const std::shared_ptr<DataFile> &file, sg_size_t payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The file to lookup */
        std::shared_ptr<DataFile> file;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to a file lookup request
     */
    class FileRegistryFileLookupAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupAnswerMessage(std::set<std::shared_ptr<FileLocation>> locations,
                                            sg_size_t payload);

        /** @brief A (possibly empty) set of file locations */
        std::set<std::shared_ptr<FileLocation>> locations;
    };

    /**
     * @brief A message sent to a FileRegistryService to request a file lookup, expecting a reply
     *        in which file locations are sorted by decreasing proximity to some reference host
     */
    class FileRegistryFileLookupByProximityRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupByProximityRequestMessage(S4U_CommPort *answer_commport, std::shared_ptr<DataFile> file,
                                                        const std::string& reference_host,
                                                        std::shared_ptr<NetworkProximityService> network_proximity_service,
                                                        sg_size_t payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The file to lookup */
        std::shared_ptr<DataFile> file;
        /**
         * @brief The host from which network proximity will be measured from.
         * If 'host_to_measure_from' is host 'A', and the workflow file resides at hosts 'B'
         * and 'C', then the proximity will be computed between hosts 'A' and 'B', and hosts
         * 'A' and 'C' so that the locations of the workflow file may be sorted with respect to
         * their current network proximity value
         */
        std::string reference_host;

        /**
         * @brief The network proximity service to be used
         */
        std::shared_ptr<NetworkProximityService> network_proximity_service;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to a file lookup request, in which
     *        file locations are sorted by decreasing proximity to some reference host
     */
    class FileRegistryFileLookupByProximityAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupByProximityAnswerMessage(std::shared_ptr<DataFile> file,
                                                       const std::string &reference_host,
                                                       std::map<double, std::shared_ptr<FileLocation>> locations,
                                                       sg_size_t payload);

        /** @brief The file to lookup */
        std::shared_ptr<DataFile> file;
        /**
         * @brief The host from which network proximity will be measured from.
         * If 'host_to_measure_from' is host 'A', and the workflow file resides at hosts 'B'
         * and 'C', then the proximity will be computed between hosts 'A' and 'B', and hosts
         * 'A' and 'C' so that the locations of the workflow file may be sorted with respect to
         * their current network proximity value
         */
        std::string reference_host;
        /**
         * @brief A map of all locations where the file resides sorted with respect to their distance from
         * the host 'host_to_measure_from'
         */
        std::map<double, std::shared_ptr<FileLocation>> locations;
    };

    /**
     * @brief A message sent to a FileRegistryService to request the removal of an entry
     */
    class FileRegistryRemoveEntryRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryRequestMessage(S4U_CommPort *answer_commport,
                                              std::shared_ptr<FileLocation> location, sg_size_t payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The location to remove */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to an entry removal request
     */
    class FileRegistryRemoveEntryAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryAnswerMessage(bool success, sg_size_t payload);

        /** @brief Whether the entry removal was successful or not */
        bool success;
    };

    /**
     * @brief A message sent to a FileRegistryService to request the addition of an entry
     */
    class FileRegistryAddEntryRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryAddEntryRequestMessage(S4U_CommPort *answer_commport,
                                           std::shared_ptr<FileLocation> location, sg_size_t payload);

        /** @brief The commport_name to which the answer message should be sent */
        S4U_CommPort *answer_commport;
        /** @brief The location in that entry */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to an entry addition request
     */
    class FileRegistryAddEntryAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryAddEntryAnswerMessage(sg_size_t payload);
    };

    /***********************/
    /** \endcond          **/
    /***********************/
}// namespace wrench


#endif//WRENCH_FILEREGISTRYMESSAGE_H
