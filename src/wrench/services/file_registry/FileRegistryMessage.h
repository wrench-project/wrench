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
#include <wrench/services/ServiceMessage.h>
#include <wrench/workflow/WorkflowFile.h>
#include <wrench/services/network_proximity/NetworkProximityService.h>
#include <wrench/services/storage/StorageService.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL    **/
    /***********************/

    /**
     * @brief Top-level FileRegistryMessage class
     */
    class FileRegistryMessage : public ServiceMessage {
    protected:
        FileRegistryMessage(std::string name, double payload);

    };

    /**
     * @brief A message sent to a FileRegistryService to request a file lookup
     */
    class FileRegistryFileLookupRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupRequestMessage(std::string answer_mailbox, WorkflowFile *file, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to lookup */
        WorkflowFile *file;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to a file lookup request
     */
    class FileRegistryFileLookupAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupAnswerMessage(WorkflowFile *file, std::set<std::shared_ptr<FileLocation>> locations,
                                            double payload);

        /** @brief The file that was looked up */
        WorkflowFile *file;
        /** @brief A (possibly empty) set of file locations */
        std::set<std::shared_ptr<FileLocation>> locations;
    };

    /**
     * @brief A message sent to a FileRegistryService to request a file lookup, expecting a reply
     *        in which file locations are sorted by decreasing proximity to some reference host
     */
    class FileRegistryFileLookupByProximityRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryFileLookupByProximityRequestMessage(std::string answer_mailbox, WorkflowFile *file,
                                                        std::string reference_host,
                                                        std::shared_ptr<NetworkProximityService> network_proximity_service,
                                                        double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file to lookup */
        WorkflowFile *file;
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
        FileRegistryFileLookupByProximityAnswerMessage(WorkflowFile *file,
                                                       std::string reference_host,
                                                       std::map<double, std::shared_ptr<FileLocation>> locations,
                                                       double payload);

        /** @brief The file to lookup */
        WorkflowFile *file;
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
        FileRegistryRemoveEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file,
                                              std::shared_ptr<FileLocation> location, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file for which one entry should be removed */
        WorkflowFile *file;
        /** @brief The location to remove */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to an entry removal request
     */
    class FileRegistryRemoveEntryAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryRemoveEntryAnswerMessage(bool success, double payload);

        /** @brief Whether the entry removal was successful or not */
        bool success;
    };

    /**
     * @brief A message sent to a FileRegistryService to request the addition of an entry
     */
    class FileRegistryAddEntryRequestMessage : public FileRegistryMessage {
    public:
        FileRegistryAddEntryRequestMessage(std::string answer_mailbox, WorkflowFile *file,
                                           std::shared_ptr<FileLocation> location, double payload);

        /** @brief The mailbox to which the answer message should be sent */
        std::string answer_mailbox;
        /** @brief The file for which to add an entry */
        WorkflowFile *file;
        /** @brief The location in that entry */
        std::shared_ptr<FileLocation> location;
    };

    /**
     * @brief A message sent by a FileRegistryService in answer to an entry addition request
     */
    class FileRegistryAddEntryAnswerMessage : public FileRegistryMessage {
    public:
        FileRegistryAddEntryAnswerMessage(double payload);
    };

    /***********************/
    /** \endcond          **/
    /***********************/
};


#endif //WRENCH_FILEREGISTRYMESSAGE_H
