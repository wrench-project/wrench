/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifdef MESSAGE_MANAGER

#ifndef WRENCH_MESSAGEMANAGER_H
#define WRENCH_MESSAGEMANAGER_H

#include <unordered_set>

#include <wrench/services/Service.h>
#include <wrench/simulation/SimulationMessage.h>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A helper class that manages messages (in terms of memory_manager_service deallocation to avoid leaks when
     *        a message was sent but never received)
     */

    class MessageManager {

        static std::unordered_map<std::string,std::unordered_set<SimulationMessage*>> mailbox_messages;

    public:

        static void manageMessage(const std::string &mailbox, SimulationMessage* msg);
        static void cleanUpMessages(const std::string &mailbox);
        static void removeReceivedMessage(const std::string &mailbox, SimulationMessage *msg);
        static void cleanUpAllMessages();
        static void print();

    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_MESSAGEMANAGER_H

#endif //MESSAGE_MANAGER