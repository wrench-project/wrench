/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "FileRegistryMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    FileRegistryMessage::FileRegistryMessage(std::string name, double payload) :
            ServiceMessage("FileRegistry::" + name, payload) {

    }

    /**
     * @brief FileRegistryFileLookupRequestMessage class
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file to look up
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupRequestMessage::FileRegistryFileLookupRequestMessage(std::string answer_mailbox,
                                                                               WorkflowFile *file, double payload) :
            FileRegistryMessage("FILE_LOOKUP_REQUEST", payload) {

      if ((answer_mailbox == "") || file == nullptr) {
        throw std::invalid_argument("FileRegistryFileLookupRequestMessage::FileRegistryFileLookupRequestMessage(): Invalid argument");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
    }

    /**
     * @brief FileRegistryFileLookupAnswerMessage class
     * @param file: the file that was looked up
     * @param locations: the set of storage services where the file is located
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupAnswerMessage::FileRegistryFileLookupAnswerMessage(WorkflowFile *file,
                                                                             std::set<StorageService *> locations,
                                                                             double payload) :
            FileRegistryMessage("FILE_LOOKUP_ANSWER", payload) {
      if (file == nullptr) {
        throw std::invalid_argument("FileRegistryFileLookupAnswerMessage::FileRegistryFileLookupAnswerMessage(): Invalid argument");
      }
      this->file = file;
      this->locations = locations;
    }

    /**
     * @brief FileRegistryFileLookupByProximityRequestMessage class
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file to look up
     * @param reference_host: the host from which network proximity will be calculated from //
     * @param network_proximity_service: a reference to the network proximity service to be used
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupByProximityRequestMessage::FileRegistryFileLookupByProximityRequestMessage(
            std::string answer_mailbox, WorkflowFile *file, std::string reference_host,
           NetworkProximityService *network_proximity_service, double payload) :
    FileRegistryMessage("FILE_LOOKUP_BY_PROXIMITY_REQUEST", payload) {
        if ((file == nullptr) || (answer_mailbox == "") || (reference_host == "") || (network_proximity_service == nullptr)) {
            throw std::invalid_argument("FileRegistryFileLookupByProximityRequestMessage::FileRegistryFileLookupByProximityRequestMessage(): Invalid Argument");
        }
        this->answer_mailbox = answer_mailbox;
        this->file = file;
        this->reference_host = reference_host;
        this->network_proximity_service = network_proximity_service;
    }

    /**
     * @brief FileRegistryFileLookupByProximityRequestMessage class
     * @param file: the file to look up
     * @param reference_host: the host from which network proximity will be calculated from // 
     * @param locations: the map of locations at which the file resides in ascending order with respect to their distance (network proximity) from 'reference_host'
     * @param payload: the message size in bytes
     */
    FileRegistryFileLookupByProximityAnswerMessage::FileRegistryFileLookupByProximityAnswerMessage(
            WorkflowFile *file, std::string reference_host,
            std::map<double, StorageService *> locations,
            double payload) :
            FileRegistryMessage("FILE_LOOKUP_BY_PROXIMITY_ANSWER", payload) {
        if ((file == nullptr) || (reference_host == "")) {
            throw std::invalid_argument(
                    "FileRegistryFileLookupByProximityAnswertMessage::FileRegistryFileLookupByProximityAnswerMessage(): Invalid Argument");
        }
        this->file = file;
        this->reference_host = reference_host;
        this->locations = locations;
    }
    /**
     * @brief FileRegistryRemoveEntryRequestMessage class
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file in the entry to remove
     * @param storage_service: the storage service in the entry to remove
     * @param payload: the message size in bytes
     */
    FileRegistryRemoveEntryRequestMessage::FileRegistryRemoveEntryRequestMessage(std::string answer_mailbox,
                                                                                 WorkflowFile *file,
                                                                                 StorageService *storage_service,
                                                                                 double payload) :
            FileRegistryMessage("REMOVE_ENTRY_REQUEST", payload) {
      if ((answer_mailbox == "") || (file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("FileRegistryRemoveEntryRequestMessage::FileRegistryRemoveEntryRequestMessage(): Invalid argument");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->storage_service = storage_service;
    }


    /**
     * @brief FileRegistryRemoveEntryAnswerMessage class
     * @param success: whether the entry removal was successful
     * @param payload: the message size in bytes
     */
    FileRegistryRemoveEntryAnswerMessage::FileRegistryRemoveEntryAnswerMessage(bool success,
                                                                               double payload) :
            FileRegistryMessage("REMOVE_ENTRY_ANSWER", payload) {
      this->success = success;
    }

    /**
     * @brief FileRegistryAddEntryRequestMessage class
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param file: the file in the entry to add
     * @param storage_service: the storage service in the entry to add
     * @param payload: the message size in bytes
     */
    FileRegistryAddEntryRequestMessage::FileRegistryAddEntryRequestMessage(std::string answer_mailbox,
                                                                           WorkflowFile *file,
                                                                           StorageService *storage_service,
                                                                           double payload) :
            FileRegistryMessage("ADD_ENTRY_REQUEST", payload) {
      if ((answer_mailbox == "") || (file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("FileRegistryAddEntryRequestMessage::FileRegistryAddEntryRequestMessage(): Invalid argument");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->storage_service = storage_service;

    }

    /**
     * @brief FileRegistryAddEntryAnswerMessage class
     * @param payload: the message size in bytes
     */
    FileRegistryAddEntryAnswerMessage::FileRegistryAddEntryAnswerMessage(double payload) :
            FileRegistryMessage("ADD_ENTRY_ANSWER", payload) {
    }

};
