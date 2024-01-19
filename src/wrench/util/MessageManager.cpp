/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifdef MESSAGE_MANAGER

#include <iostream>
#include <wrench/logging/TerminalOutput.h>

#include <wrench/util/MessageManager.h>

WRENCH_LOG_CATEGORY(wrench_core_message_manager, "Log category for MessageManager");


namespace wrench {

    // TODO: At some point, we may want to make this with only unique pointers...

    std::unordered_map<const S4U_CommPort *, std::unordered_set<SimulationMessage *>> MessageManager::messages = {};

    /**
     * @brief Insert a message in the manager's  "database"
     * @param commport: the name of the relevant commport
     * @param msg: the message
     * @throw std::runtime_error
     */
    void MessageManager::manageMessage(const S4U_CommPort *commport, SimulationMessage *msg) {
        if (msg == nullptr) {
            throw std::runtime_error(
                    "MessageManager::manageMessage()::Null Message cannot be managed by MessageManager");
        }
        if (messages.find(commport) == messages.end()) {
            messages.insert({commport, {}});
        }
        messages[commport].insert(msg);
        //      WRENCH_INFO("MESSAGE_MANAGER: INSERTING [%s]:%s (%lu)", commport.c_str(), msg->getName().c_str(), (unsigned long)msg);
    }

    /**
     * @brief Clean up messages for a given commport (so as to free up memory_manager_service)
     * @param commport: the commport name
     */
    void MessageManager::cleanUpMessages(const S4U_CommPort *commport) {
        if (messages.find(commport) != messages.end()) {
            for (auto msg: messages[commport]) {
                delete msg;
            }
            messages[commport].clear();
            messages.erase(commport);
        }
    }

    /**
     * @brief Clean up all the messages that MessageManager has stored (so as to free up memory_manager_service)
     */
    void MessageManager::cleanUpAllMessages() {
        for (auto const &m: messages) {
            cleanUpMessages(m.first);
        }
    }

    /**
     * @brief A debug function to print the content of the message manager
     */
    void MessageManager::print() {
        WRENCH_INFO("MessageManager DB:");
        for (auto const &m: messages) {
            WRENCH_INFO("   ==> [%s]:%lu", m.first->get_cname(), m.second.size());
        }
    }

    /**
     * @brief Remove a received message from the "database" of messages
     * @param commport: the name of the commport from which the message was received
     * @param msg: the message
     */
    void MessageManager::removeReceivedMessage(const S4U_CommPort *commport, SimulationMessage *msg) {
        //      if (messages.find(commport) != messages.end()) {
        //        if (messages[commport].find(msg) != messages[commport].end()) {
        //            WRENCH_INFO("MESSAGE_MANAGER: REMOVING [%s]:%s (%lu)", commport.c_str(), msg->getName().c_str(), (unsigned long)msg);
        messages[commport].erase(msg);
        //          if (messages[commport].empty()) {
        //              messages.erase(commport);
        //          }
        //        }
        //      }
    }
}// namespace wrench

#endif//MESSAGE_MANAGER
