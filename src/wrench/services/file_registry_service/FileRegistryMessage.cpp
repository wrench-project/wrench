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

    FileRegistryMessage::FileRegistryMessage(std::string name, double payload) :
            ServiceMessage("FileRegistry::" + name, payload) {

    }

    FileRegistryFileLookupRequestMessage::FileRegistryFileLookupRequestMessage(std::string answer_mailbox,
                                                                               WorkflowFile *file, double payload) :
            FileRegistryMessage("FILE_LOOKUP_REQUEST", payload) {

      if ((answer_mailbox == "") || file == nullptr) {
        throw std::invalid_argument("Invalid constructor argument");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
    }

    FileRegistryFileLookupAnswerMessage::FileRegistryFileLookupAnswerMessage(WorkflowFile *file,
                                                                             std::set<StorageService *> locations,
                                                                             double payload) :
            FileRegistryMessage("FILE_LOOKUP_ANSWER", payload) {
      if (file == nullptr) {
        throw std::invalid_argument("Invalid constructor argument");
      }
      this->file = file;
      this->locations = locations;
    }


    FileRegistryRemoveEntryRequestMessage::FileRegistryRemoveEntryRequestMessage(std::string answer_mailbox,
                                                                                 WorkflowFile *file,
                                                                                 StorageService *storage_service,
                                                                                 double payload) :
            FileRegistryMessage("REMOVE_ENTRY_REQUEST", payload) {
      if ((answer_mailbox == "") || (file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Invalid constructor argument");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->storage_service = storage_service;
    }


    FileRegistryRemoveEntryAnswerMessage::FileRegistryRemoveEntryAnswerMessage(bool success,
                                                                               double payload) :
            FileRegistryMessage("REMOVE_ENTRY_ANSWER", payload) {
      this->success = success;
    }

    FileRegistryAddEntryRequestMessage::FileRegistryAddEntryRequestMessage(std::string answer_mailbox,
                                                                           WorkflowFile *file,
                                                                           StorageService *storage_service,
                                                                           double payload) :
            FileRegistryMessage("ADD_ENTRY_REQUEST", payload) {
      if ((answer_mailbox == "") || (file == nullptr) || (storage_service == nullptr)) {
        throw std::invalid_argument("Invalid constructor argument");
      }
      this->answer_mailbox = answer_mailbox;
      this->file = file;
      this->storage_service = storage_service;

    }

    FileRegistryAddEntryAnswerMessage::FileRegistryAddEntryAnswerMessage(double payload) :
            FileRegistryMessage("ADD_ENTRY_ANSWER", payload) {
    }

};