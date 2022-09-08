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
#include "wrench/services/storage/StorageServiceMessage.h"

namespace wrench {
    namespace XRootD {

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        class Node;

        /**
         * @brief Top-level class for messages received/sent by a XRootD Node
         */
        class Message : public StorageServiceMessage {
        protected:
            Message(double payload);
        };
        /**
         * @brief A message sent to a XRootD Node to continue an ongoing search for a file
         */
        class ContinueSearchMessage : public Message {
        public:
            ContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                  std::shared_ptr<StorageServiceFileReadRequestMessage> original,
                                  std::shared_ptr<DataFile> file,
                                  Node *node,
                                  double payload,
                                  std::shared_ptr<bool> answered,
                                  int timeToLive);
            ContinueSearchMessage(ContinueSearchMessage *toCopy);
            /** @brief Mailbox to which the FINAL answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;

            /** @brief The original file read request that kicked off the search (if null this was a lookup request)*/
            std::shared_ptr<StorageServiceFileReadRequestMessage> original;

            /** @brief The file being searched for */
            std::shared_ptr<DataFile> file;

            /** The node that originally received the FileLookupRequest or FileReadRequest */
            Node *node;
            /** @brief Whether or not the calling client has been answered yet.  Used to prevent answer_mailbox spamming for multiple file hits */
            std::shared_ptr<bool> answered;
            /** How many more hops this message can live for, to prevent messages living forever in improper configurations with loops.*/
            int timeToLive;
        };

        /**
        * @brief A message sent to a XRootD Node to update the cache
        */

        class UpdateCacheMessage : public Message {
        public:
            UpdateCacheMessage(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<StorageServiceFileReadRequestMessage> original, Node *node, std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations,
                               double payload, std::shared_ptr<bool> answered);
            UpdateCacheMessage(UpdateCacheMessage &other);
            UpdateCacheMessage(UpdateCacheMessage *other);
            /** @brief Mailbox to which the FINAL answer message should be sent */
            simgrid::s4u::Mailbox *answer_mailbox;
            /** @brief The original file read request that kicked off the search (if null this was a lookup request)*/
            std::shared_ptr<StorageServiceFileReadRequestMessage> original;
            /** @brief The file found */
            std::shared_ptr<DataFile> file;
            /** @brief the locations to cache */
            set<std::shared_ptr<FileLocation>> locations;
            /** @brief The highest node in the tree to return to when caching (should be the node the original message was sent) */
            Node *node;
            /** @brief Whether or not the calling client has been answered yet.  Used to prevent answer_mailbox spamming for multiple file hits */
            std::shared_ptr<bool> answered;
        };

        /**
         * @brief A message sent to a XRootD Node to delete a file
         */
        class RippleDelete : public Message {
        public:
            RippleDelete(std::shared_ptr<DataFile> file, double payload, int timeToLive);
            RippleDelete(RippleDelete *other);
            RippleDelete(StorageServiceFileDeleteRequestMessage *other, int timeToLive);
            /** @brief The file to delete */
            std::shared_ptr<DataFile> file;
            /** @brief The remaining hops before the message should no longer be perpetuated */
            int timeToLive;
        };

        /**
         * @brief A message sent to a XRootD Node to delete a file
         */
        class AdvancedContinueSearchMessage : public ContinueSearchMessage {
        public:
            AdvancedContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                          std::shared_ptr<StorageServiceFileReadRequestMessage> original,
                                          std::shared_ptr<DataFile> file,
                                          Node *node,
                                          double payload,
                                          std::shared_ptr<bool> answered,
                                          int timeToLive,
                                          std::vector<std::stack<Node *>> search_stack);
            AdvancedContinueSearchMessage(AdvancedContinueSearchMessage *toCopy);
            AdvancedContinueSearchMessage(ContinueSearchMessage *toCopy, std::vector<std::stack<Node *>> search_stack);

            /** @brief The paths to follow */
            std::vector<std::stack<Node *>> search_stack;
        };

        /**
         *
         * @brief A message sent to a XRootD Node to delete a file
         */
        class AdvancedRippleDelete : public RippleDelete {
        public:
            AdvancedRippleDelete(std::shared_ptr<DataFile> file, double payload, int timeToLive, std::vector<std::stack<Node *>> search_stack);
            AdvancedRippleDelete(AdvancedRippleDelete *other);
            AdvancedRippleDelete(StorageServiceFileDeleteRequestMessage *other, int timeToLive, std::vector<std::stack<Node *>> search_stack);
            AdvancedRippleDelete(RippleDelete *other, std::vector<std::stack<Node *>> search_stack);

            /** @brief The paths to follow */
            std::vector<std::stack<Node *>> search_stack;
        };


        /***********************/
        /** \endcond           */
        /***********************/
    }// namespace XRootD
};   // namespace wrench


#endif//WRENCH_XRootDMessage_H
