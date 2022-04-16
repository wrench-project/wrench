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
#include "wrench/simulation/SimulationOutput.h"]
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
        * @brief A message sent to a XRootD Node to lookup a file
        */
        class ContinueSearchMessage : public Message {
        public:
            ContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox,simgrid::s4u::Mailbox *block_mailbox,   std::vector<std::shared_ptr<SearchStack>> path,
                                                    double payload);

            /** @brief Mailbox to which the answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;

            /** @brief The mailbox to which the file content should be sent */
            simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content;
            /** @brief Nodes to search */
            std::vector<std::shared_ptr<SearchStack>> path;
            /** @brief The message to hand off to any file servers in the path */
            std::shared_ptr<Message> message;
        };
        class UpdateCacheMessage : public Message {
        public:
            UpdateCacheMessage(std::shared_ptr<DataFile> file, std::shared_ptr<SearchStack> stack, std::shared_ptr<FileLocation> location,
                                  double payload);

            /** @brief The highest node in the tree to return to when caching (should be the node the original message was sent) */
            std::shared_ptr<SearchStack> stack;
            /** @brief The file found */
            std::shared_ptr<DataFile> file;
            /** @brief the location to cache */
            std::shared_ptr<FileLocation> location;
        };

        /**
               * @brief A message sent to a XRootD Node to lookup a file
               */
        class FileLookupRequestMessage : public Message {
        public:
            FileLookupRequestMessage(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<DataFile> file,
                                                   double payload);

            /** @brief Mailbox to which the answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;
            /** @brief The file to lookup */
            std::shared_ptr<DataFile> file;
        };

        /**
         * @brief A message sent to a XRootD Node to delete a file
         */
        class FileDeleteRequestMessage : public Message {
        public:
            FileDeleteRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                   std::shared_ptr<DataFile> file,
                                                   double payload);

            /** @brief Mailbox to which the answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;
            /** @brief The file to delete */
            std::shared_ptr<DataFile> file;
        };

        /**
         * @brief A message sent to a XRootD Node to read a file
         */
        class FileReadRequestMessage : public Message {
        public:
            FileReadRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                 simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content,
                                                 std::shared_ptr<DataFile> file,
                                                 double num_bytes_to_read,
                                                 unsigned long buffer_size,
                                                 double payload);

            /** @brief The mailbox to which the answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;
            /** @brief The mailbox to which the file content should be sent */
            simgrid::s4u::Mailbox *mailbox_to_receive_the_file_content;
            /** @brief The file to read */
            std::shared_ptr<DataFile> file;
            /** @brief The number of bytes to read */
            double num_bytes_to_read;
            /** @brief The requested buffer size */
            unsigned long buffer_size;
        };
        class FileReadAnswerMessage : public Message {
        public:
            FileReadAnswerMessage(std::shared_ptr<DataFile> file,
                                  std::shared_ptr<FileLocation> location,
                                  bool success,
                                  std::shared_ptr<FailureCause> failure_cause,
                                  double payload);




        };



        /***********************/
        /** \endcond           */
        /***********************/
    }
};// namespace wrench


#endif//WRENCH_XRootDMessage_H
