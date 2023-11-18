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

    std::unordered_map<std::string, std::unordered_set<SimulationMessage *>> MessageManager::mailbox_messages = {};

    /**
     * @brief Insert a message in the manager's  "database"
     * @param commport_name: the name of the relevant commport_name
     * @param msg: the message
     * @throw std::runtime_error
     */
    void MessageManager::manageMessage(const std::string &commport_name, SimulationMessage *msg) {
        if (msg == nullptr) {
            throw std::runtime_error(
                    "MessageManager::manageMessage()::Null Message cannot be managed by MessageManager");
        }
        if (mailbox_messages.find(commport_name) == mailbox_messages.end()) {
            mailbox_messages.insert({commport_name, {}});
        }
        mailbox_messages[commport_name].insert(msg);
        //      WRENCH_INFO("MESSAGE_MANAGER: INSERTING [%s]:%s (%lu)", commport_name.c_str(), msg->getName().c_str(), (unsigned long)msg);
    }

    /**
     * @brief Clean up messages for a given commport_name (so as to free up memory_manager_service)
     * @param commport_name: the commport_name name
     */
    void MessageManager::cleanUpMessages(const std::string &commport_name) {
        if (mailbox_messages.find(commport_name) != mailbox_messages.end()) {
            for (auto msg: mailbox_messages[commport_name]) {
                delete msg;
            }
            mailbox_messages[commport_name].clear();
            mailbox_messages.erase(commport_name);
        }
    }

    /**
     * @brief Clean up all the messages that MessageManager has stored (so as to free up memory_manager_service)
     */
    void MessageManager::cleanUpAllMessages() {
        for (auto m: mailbox_messages) {
            cleanUpMessages(m.first);
        }
    }

    /**
     * @brief A debug function to print the content of the message manager
     */
    void MessageManager::print() {
        WRENCH_INFO("MessageManager DB:");
        for (auto const &x: mailbox_messages) {
            WRENCH_INFO("   ==> [%s]:%lu", x.first.c_str(), x.second.size());
        }
    }

    /**
     * @brief Remove a received message from the "database" of messages
     * @param commport_name: the name of the commport_name from which the message was received
     * @param msg: the message
     */
    void MessageManager::removeReceivedMessage(const std::string &commport_name, SimulationMessage *msg) {
        //      if (mailbox_messages.find(commport_name) != mailbox_messages.end()) {
        //        if (mailbox_messages[commport_name].find(msg) != mailbox_messages[commport_name].end()) {
        //            WRENCH_INFO("MESSAGE_MANAGER: REMOVING [%s]:%s (%lu)", commport_name.c_str(), msg->getName().c_str(), (unsigned long)msg);
        mailbox_messages[commport_name].erase(msg);
        //          if (mailbox_messages[commport_name].empty()) {
        //              mailbox_messages.erase(commport_name);
        //          }
        //        }
        //      }

#if 0
        std::map<std::string, std::vector<SimulationMessage *>>::iterator msg_itr;
        for (msg_itr = mailbox_messages.begin(); msg_itr != mailbox_messages.end(); msg_itr++) {
          if ((*msg_itr).first == commport_name) {
            std::vector<SimulationMessage *>::iterator it;
            for (it = (*msg_itr).second.begin(); it != (*msg_itr).second.end(); it++) {
              if ((*it) == msg) {
                it = (*msg_itr).second.erase(it);
                break;
              }
            }
          }
        }
#endif
    }
}// namespace wrench

#endif//MESSAGE_MANAGER
