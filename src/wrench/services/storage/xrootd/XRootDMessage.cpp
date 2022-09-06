/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "wrench/services/storage/xrootd/XRootDMessage.h"

/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


namespace wrench {
    namespace XRootD {
        /**
         * @brief Constructor
         * @param payload: the message size in bytes
         */
        Message::Message(double payload) : StorageServiceMessage(payload) {}
        /**
         * @brief Constructor
         * @param answer_mailbox: The mailbox the final answer should be sent to
         * @param original: The original file read request being responded too.  If this is a file locate search, this should be null
         * @param file: The file to search for
         * @param node: The node where the search was initiated
         * @param payload: The message size in bytes
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem
         * @param timeToLive: The max number of hops this message can take
         */
        ContinueSearchMessage::ContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                                     std::shared_ptr<StorageServiceFileReadRequestMessage> original,
                                                     std::shared_ptr<DataFile> file,
                                                     Node *node,
                                                     double payload,
                                                     std::shared_ptr<bool> answered,
                                                     int timeToLive) : Message(payload), answer_mailbox(answer_mailbox), original(original), file(file), node(node), answered(answered), timeToLive(timeToLive) {}
        /**
        * @brief Copy Constructor
        * @param other: The message to copy.  timeToLive is decremented
        */
        ContinueSearchMessage::ContinueSearchMessage(ContinueSearchMessage *other) : Message(other->payload), answer_mailbox(other->answer_mailbox), original(other->original), file(other->file), node(other->node), answered(other->answered), timeToLive(other->timeToLive - 1) {}

        /**
         * @brief Constructor
         * @param answer_mailbox: The mailbox the final answer should be sent to
         * @param original: The original file read request being responded too.  If this is a file locate search, this should be null
         * @param node: The node where the search was initiated
         * @param file: The file that was found
         * @param locations: All locations that where found in this subtree
         * @param payload: The message size in bytes
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem
         */
        UpdateCacheMessage::UpdateCacheMessage(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<StorageServiceFileReadRequestMessage> original, Node *node, std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations,
                                               double payload, std::shared_ptr<bool> answered) : Message(payload), answer_mailbox(answer_mailbox), original(original), file(file), locations(locations), node(node), answered(answered) {}
        /**
        * @brief Pointer Copy Constructor
        * @param other: The message to copy.
        */
        UpdateCacheMessage::UpdateCacheMessage(UpdateCacheMessage *other) : UpdateCacheMessage(*other) {}
        /**
        * @brief Reference Copy Constructor
        * @param other: The message to copy.
        */
        UpdateCacheMessage::UpdateCacheMessage(UpdateCacheMessage &other) : Message(other.payload), answer_mailbox(other.answer_mailbox), original(other.original), file(other.file), locations(other.locations), node(other.node), answered(other.answered) {}
        /**
        * @brief Constructor
        * @param file: The file to delete.
        * @param payload: the message size in bytes
        * @param timeToLive:  The max number of hops this message can take
        */
        RippleDelete::RippleDelete(std::shared_ptr<DataFile> file, double payload, int timeToLive) : Message(payload), file(file), timeToLive(timeToLive){};
        /**
        * @brief Copy Constructor
        * @param other: The message to copy.
        */
        RippleDelete::RippleDelete(RippleDelete *other) : Message(other->payload), file(other->file), timeToLive(other->timeToLive - 1) {}

        /**
        * @brief External Copy Constructor
        * @param other: The storage service file delete message to copy.
        * @param timeToLive:  The max number of hops this message can take
        */
        RippleDelete::RippleDelete(StorageServiceFileDeleteRequestMessage *other, int timeToLive) : Message(other->payload), file(other->file), timeToLive(timeToLive) {}

        /**
         * @brief Constructor
         * @param answer_mailbox: The mailbox the final answer should be sent to
         * @param original: The original file read request being responded too.  If this is a file locate search, this should be null
         * @param file: The file to search for
         * @param node: The node where the search was initiated
         * @param payload: The message size in bytes
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem
         * @param timeToLive: The max number of hops this message can take
         * @param search_stack:  The available paths to the file
         */
        AdvancedContinueSearchMessage::AdvancedContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<StorageServiceFileReadRequestMessage> original,
                                                                     std::shared_ptr<DataFile> file, Node *node, double payload, std::shared_ptr<bool> answered, int timeToLive, std::vector<std::stack<Node *>> search_stack) : ContinueSearchMessage(answer_mailbox, original, file, node, payload, answered, timeToLive), search_stack(search_stack){};
        /**
        * @brief Pointer Copy Constructor with auxiliary stack
        * @param toCopy: The message to copy, timeToLive is decremented
        * @param search_stack:  The available paths to the file
        */
        AdvancedContinueSearchMessage::AdvancedContinueSearchMessage(ContinueSearchMessage *toCopy, std::vector<std::stack<Node *>> search_stack) : ContinueSearchMessage(toCopy), search_stack(search_stack){};

        /**
        * @brief Pointer Copy Constructor
        * @param toCopy: The message to copy, timeToLive is decremented
        */
        AdvancedContinueSearchMessage::AdvancedContinueSearchMessage(AdvancedContinueSearchMessage *toCopy) : ContinueSearchMessage(toCopy), search_stack(toCopy->search_stack){};

        /**
        * @brief Constructor
        * @param file: The file to delete.
        * @param payload: the message size in bytes
        * @param timeToLive:  The max number of hops this message can take
        * @param search_stack:  The available paths to the file
        */
        AdvancedRippleDelete::AdvancedRippleDelete(std::shared_ptr<DataFile> file, double payload, int timeToLive, std::vector<std::stack<Node *>> search_stack) : RippleDelete(file, payload, timeToLive), search_stack(search_stack) {}

        /**
        * @brief Copy Constructor with auxiliary stack
        * @param other: The message to copy.
        * @param search_stack:  The available paths to the file
        */
        AdvancedRippleDelete::AdvancedRippleDelete(RippleDelete *other, std::vector<std::stack<Node *>> search_stack) : RippleDelete(other), search_stack(search_stack){};

        /**
        * @brief Copy Constructor
        * @param other: The message to copy.
        */
        AdvancedRippleDelete::AdvancedRippleDelete(AdvancedRippleDelete *other) : RippleDelete(other), search_stack(other->search_stack){};

        /**
         * @brief External Copy Constructor
         * @param other: The storage service file delete message to copy.
         * @param timeToLive:  The max number of hops this message can take
         * @param search_stack:  The available paths to the file
         */
        AdvancedRippleDelete::AdvancedRippleDelete(StorageServiceFileDeleteRequestMessage *other, int timeToLive, std::vector<std::stack<Node *>> search_stack) : RippleDelete(other, timeToLive), search_stack(search_stack){};
    }// namespace XRootD
};   // namespace wrench
