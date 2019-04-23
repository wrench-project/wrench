/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

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
     * @brief A helper class that manages messages (in terms of memory deallocation to avoid leaks when
     *        a message was sent but never received)
     */

    class MessageManager {

        static std::map<std::string,std::unordered_set<SimulationMessage*>> mailbox_messages;

    public:

        static void manageMessage(std::string, SimulationMessage* msg);
        static void cleanUpMessages(std::string);
        static void removeReceivedMessage(std::string mailbox, SimulationMessage *msg);
        static void cleanUpAllMessages();

    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_MESSAGEMANAGER_H
