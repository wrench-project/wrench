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
#include "wrench/logging/TerminalOutput.h"

#include "wrench/util/MessageManager.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(message_manager, "Log category for MessageManager");


namespace wrench {

    // TODO: At some point, we may want to make this with only unique pointers...

    std::unordered_map<std::string, std::unordered_set<SimulationMessage *>> MessageManager::mailbox_messages = {};

    /**
     * @brief Insert a message in the manager's  "database"
     * @param mailbox: the name of the relevant mailbox
     * @param msg: the message
     * @throw std::runtime_error
     */
    void MessageManager::manageMessage(const std::string &mailbox, SimulationMessage *msg) {
        if (msg == nullptr) {
            throw std::runtime_error(
                    "MessageManager::manageMessage()::Null Message cannot be managed by MessageManager"
            );
        }
        if (mailbox_messages.find(mailbox) == mailbox_messages.end()) {
            mailbox_messages.insert({mailbox, {}});
        }
        mailbox_messages[mailbox].insert(msg);
//      WRENCH_INFO("MESSAGE_MANAGER: INSERTING [%s]:%s (%lu)", mailbox.c_str(), msg->getName().c_str(), (unsigned long)msg);
    }

    /**
     * @brief Clean up messages for a given mailbox (so as to free up memory)
     * @param mailbox: the mailbox name
     */
    void MessageManager::cleanUpMessages(const std::string &mailbox) {
        if (mailbox_messages.find(mailbox) != mailbox_messages.end()) {
            for (auto msg : mailbox_messages[mailbox]) {
                delete msg;
            }
            mailbox_messages[mailbox].clear();
            mailbox_messages.erase(mailbox);
        }
    }

    /**
     * @brief Clean up all the messages that MessageManager has stored (so as to free up memory)
     */
    void MessageManager::cleanUpAllMessages() {
        for (auto m : mailbox_messages) {
            cleanUpMessages(m.first);
        }
    }

    /**
     * @brief A debug function to print the content of the message manager
     */
    void MessageManager::print() {
        WRENCH_INFO("MessageManager DB:");
        for (auto const &x : mailbox_messages) {
            WRENCH_INFO("   ==> [%s]:%lu", x.first.c_str(), x.second.size());
        }
    }

    /**
     * @brief Remove a received message from the "database" of messages
     * @param mailbox: the name of the mailbox from which the message was received
     * @param msg: the message
     */
    void MessageManager::removeReceivedMessage(const std::string &mailbox, SimulationMessage *msg) {
//      if (mailbox_messages.find(mailbox) != mailbox_messages.end()) {
//        if (mailbox_messages[mailbox].find(msg) != mailbox_messages[mailbox].end()) {
//            WRENCH_INFO("MESSAGE_MANAGER: REMOVING [%s]:%s (%lu)", mailbox.c_str(), msg->getName().c_str(), (unsigned long)msg);
        mailbox_messages[mailbox].erase(msg);
//          if (mailbox_messages[mailbox].empty()) {
//              mailbox_messages.erase(mailbox);
//          }
//        }
//      }

#if 0
        std::map<std::string, std::vector<SimulationMessage *>>::iterator msg_itr;
        for (msg_itr = mailbox_messages.begin(); msg_itr != mailbox_messages.end(); msg_itr++) {
          if ((*msg_itr).first == mailbox) {
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
}

#endif //MESSAGE_MANAGER
