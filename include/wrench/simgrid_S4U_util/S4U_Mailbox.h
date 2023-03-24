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
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <simgrid/s4u.hpp>
#include <wrench/simulation/SimulationMessage.h>
namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    //class SimulationMessage;
    class S4U_PendingCommunication;

    /**
     * @brief Wrappers around S4U's communication methods
     */
    class S4U_Mailbox {

    public:
        /**
     * @brief Synchronously receive a message from a mailbox
     *
     * @param mailbox: the mailbox
     * @param error_prefix: any string you wish to prefix the error message with
     * @return the message, in a unique_ptr of the type specified.  Otherwise throws a runtime_error
     *
     * @throw std::shared_ptr<NetworkError>
     */
        template<class TMessageType>
        static std::unique_ptr<TMessageType> getMessage(simgrid::s4u::Mailbox *mailbox, const std::string &error_prefix = "") {
            auto id = ++messageCounter;
#ifndef NDEBUG
            char const *name = typeid(TMessageType).name();
            std::string tn = boost::core::demangle(name);
            templateWaitingLog(mailbox, tn, id);
#endif


            auto message = S4U_Mailbox::getMessage(mailbox, false);

            if (auto msg = dynamic_cast<TMessageType *>(message.get())) {
#ifndef NDEBUG
                templateWaitingLogUpdate(mailbox, tn, id);
#endif
                message.release();
                return std::unique_ptr<TMessageType>(msg);
            } else {
                char const *name = typeid(TMessageType).name();
                std::string tn = boost::core::demangle(name);
                throw std::runtime_error(error_prefix + " Unexpected [" + message->getName() + "] message while waiting for " + tn.c_str() + ". Request ID: " + std::to_string(id));
            }
        }
        /**
     * @brief Synchronously receive a message from a mailbox
     *
     * @param mailbox: the mailbox
     * @param error_prefix: any string you wish to prefix the error message with
     * @param timeout:  a timeout value in seconds (<0 means never timeout)
     *
     * @return the message, in a unique_ptr of the type specified.  Otherwise throws a runtime_error
     *
     * @throw std::shared_ptr<NetworkError>
     */
        template<class TMessageType>
        static std::unique_ptr<TMessageType> getMessage(simgrid::s4u::Mailbox *mailbox, double timeout, const std::string &error_prefix = "") {
            auto id = ++messageCounter;
#ifndef NDEBUG
            char const *name = typeid(TMessageType).name();
            std::string tn = boost::core::demangle(name);
            templateWaitingLog(mailbox, tn, id);
#endif


            auto message = S4U_Mailbox::getMessage(mailbox, timeout, false);

            if (auto msg = dynamic_cast<TMessageType *>(message.get())) {
                message.release();
#ifndef NDEBUG
                templateWaitingLogUpdate(mailbox, tn, id);
#endif
                return std::unique_ptr<TMessageType>(msg);
            } else {
                char const *name = typeid(TMessageType).name();
                std::string tn = boost::core::demangle(name);
                throw std::runtime_error(error_prefix + " Unexpected [" + message->getName() + "] message while waiting for " + tn.c_str() + ". Request ID: " + std::to_string(id));
            }
        }
        /**
     * @brief Synchronously receive a message from a mailbox
     *
     * @param mailbox: the mailbox
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     */
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox) {
            return getMessage(mailbox, true);
        }
        /**
     * @brief Synchronously receive a message from a mailbox, with a timeout
     *
     * @param mailbox: the mailbox
     * @param timeout:  a timeout value in seconds (<0 means never timeout)
     * @return the message, or nullptr (in which case it's likely a brutal termination)
     *
     * @throw std::shared_ptr<NetworkError>
     */
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox, double timeout) {
            return getMessage(mailbox, timeout, true);
        }
        static void putMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *m);
        static void dputMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg);
        static std::shared_ptr<S4U_PendingCommunication> iputMessage(simgrid::s4u::Mailbox *mailbox, SimulationMessage *msg);
        static std::shared_ptr<S4U_PendingCommunication> igetMessage(simgrid::s4u::Mailbox *mailbox);

        static unsigned long generateUniqueSequenceNumber();

        static simgrid::s4u::Mailbox *getTemporaryMailbox();
        static void retireTemporaryMailbox(simgrid::s4u::Mailbox *mailbox);

        static void createMailboxPool(unsigned long num_mailboxes);

        static simgrid::s4u::Mailbox *generateUniqueMailbox(const std::string &prefix);

        /**
         * @brief The mailbox pool size
         */
        static unsigned long mailbox_pool_size;

        /**
         * @brief The "not a mailbox" mailbox, to avoid getting answers back when asked
         *        to prove an "answer mailbox"
         */
        static simgrid::s4u::Mailbox *NULL_MAILBOX;


    private:
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox, bool log);
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox, double timeout, bool log);
        static std::deque<simgrid::s4u::Mailbox *> free_mailboxes;
        static std::set<simgrid::s4u::Mailbox *> used_mailboxes;
        static std::deque<simgrid::s4u::Mailbox *> mailboxes_to_drain;
        static void templateWaitingLog(const simgrid::s4u::Mailbox *mailbox, std::string type, unsigned long long id);
        static void templateWaitingLogUpdate(const simgrid::s4u::Mailbox *mailbox, std::string type, unsigned long long id);
        static unsigned long long messageCounter;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_S4U_MAILBOX_H
