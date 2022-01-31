/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_S4U_MAILBOX_H
#define WRENCH_S4U_MAILBOX_H


#include <string>
#include <map>
#include <set>

#include <simgrid/s4u.hpp>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class SimulationMessage;
    class S4U_PendingCommunication;

    /**
     * @brief Wrappers around S4U's communication methods
     */
    class S4U_Mailbox {

    public:
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox);
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox, double timeout);
        static void putMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *m);
        static void dputMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg);
        static std::shared_ptr<S4U_PendingCommunication> iputMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg);
        static std::shared_ptr<S4U_PendingCommunication> igetMessage(simgrid::s4u::Mailbox *mailbox);

        static unsigned long generateUniqueSequenceNumber();

        static simgrid::s4u::Mailbox *getTemporaryMailbox();
        static void retireTemporaryMailbox(simgrid::s4u::Mailbox *mailbox);

        static void createMailboxPool(unsigned long num_mailboxes);

        static simgrid::s4u::Mailbox *generateUniqueMailbox(std::string prefix);

        static unsigned long mailbox_pool_size;

    private:
        static std::deque<simgrid::s4u::Mailbox *> free_mailboxes;
        static std::set<simgrid::s4u::Mailbox *> used_mailboxes;
        static std::deque<simgrid::s4u::Mailbox *> mailboxes_to_drain;


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_S4U_MAILBOX_H
