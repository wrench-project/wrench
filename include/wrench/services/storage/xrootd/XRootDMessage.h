//
// Created by jamcdonald on 3/28/22.
//

/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_XRootDMessage_H
#define WRENCH_XRootDMessage_H


#include <memory>

#include "wrench/services/ServiceMessage.h"
#include "wrench/failure_causes/FailureCause.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/simulation/SimulationTimestampTypes.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/simulation/SimulationOutput.h"
#include "SearchStack.h"

namespace wrench {
    namespace XRootD{
        /***********************/
        /** \cond INTERNAL     */
        /***********************/




        /**
         * @brief Top-level class for messages received/sent by a XRootD Node
         */
         class Node;
        class Message : public ServiceMessage {
        protected:
            Message(double payload);
        };

        /**
         * @brief A message sent to a XRootD Node to read a file
         */
        class FileSearchRequestMessage : public Message {
        public:
            FileSearchRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                   std::shared_ptr<DataFile> file,
                                   double payload);

            /** @brief The mailbox to which the answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;
            /** @brief The file to read */
            std::shared_ptr<DataFile> file;
            /** @brief The number of bytes to read */
            double num_bytes_to_read;
        };

        class FileSearchAnswerMessage : public Message {
        public:
            std::shared_ptr<DataFile> file;
            std::shared_ptr<FileLocation> location;
            bool success;
            std::shared_ptr<FailureCause> failure_cause;
            double payload;
            FileSearchAnswerMessage(std::shared_ptr<DataFile> file,
                                  std::shared_ptr<FileLocation> location,
                                  bool success,
                                  std::shared_ptr<FailureCause> failure_cause,
                                  double payload);




        };
        class ContinueSearchMessage : public Message {
        public:
            ContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                  std::shared_ptr<DataFile> file,
                                  Node* node,
                                  double payload,
                                  std::shared_ptr<bool> answered,
                                  int timeToLive);
            ContinueSearchMessage(ContinueSearchMessage* toCopy);
            /** @brief Mailbox to which the FINAL answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;

            /** @brief The file being searched for */
            std::shared_ptr<DataFile> file;

            /** The node that oriinaly received the FileLookupRequest or FileReadRequest */
            Node* node;
            /** @brief Whether or not the calling client has been answered yet.  Used to prevent answer_mailbox spamming for multiple file hits */
            std::shared_ptr<bool> answered;
            /** How many more hops this message can live for, to prevent messages living forever in impropper configurations with loops.*/
            int timeToLive;
        };

        /**
        * @brief A message sent to a XRootD Node to update the cache
        */

        class UpdateCacheMessage : public Message {
        public:
            UpdateCacheMessage(simgrid::s4u::Mailbox *answer_mailbox,Node* node,std::shared_ptr<DataFile> file,  std::set<std::shared_ptr<FileLocation>> locations,
                               double payload, std::shared_ptr<bool> answered);

            /** @brief Mailbox to which the FINAL answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;
            /** @brief The file found */
            std::shared_ptr<DataFile> file;
            /** @brief the locations to cache */
            set<std::shared_ptr<FileLocation>> locations;
            /** @brief The highest node in the tree to return to when caching (should be the node the original message was sent) */
            Node* node;
            /** @brief Whether or not the calling client has been answered yet.  Used to prevent answer_mailbox spamming for multiple file hits */
            std::shared_ptr<bool> answered;
        };

        /**
         * @brief A message sent to a XRootD Node to delete a file
         */
        class FileDeleteRequestMessage : public Message {
        public:
            FileDeleteRequestMessage(              std::shared_ptr<DataFile> file,
                                                   double payload,int timeToLive);
            FileDeleteRequestMessage(FileDeleteRequestMessage* other);
            /** @brief The file to delete */
            std::shared_ptr<DataFile> file;
            int timeToLive;
        };





        /***********************/
        /** \endcond           */
        /***********************/
    }
};// namespace wrench


#endif//WRENCH_XRootDMessage_H
