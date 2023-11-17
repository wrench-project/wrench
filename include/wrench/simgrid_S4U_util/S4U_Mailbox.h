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
         * @brief Constructor
         */
        S4U_Mailbox() {
            this->s4u_mb = simgrid::s4u::Mailbox::by_name("tmp" + std::to_string(S4U_Mailbox::generateUniqueSequenceNumber()));
            this->name = this->s4u_mb->get_name();
        }

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
        std::unique_ptr<TMessageType> getMessage(const std::string &error_prefix = "") {
            auto id = ++messageCounter;
#ifndef NDEBUG
            char const *name = typeid(TMessageType).name();
            std::string tn = boost::core::demangle(name);
            this->templateWaitingLog(tn, id);
#endif


            auto message = this->getMessage(false);

            if (auto msg = dynamic_cast<TMessageType *>(message.get())) {
#ifndef NDEBUG
                this->templateWaitingLogUpdate(tn, id);
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
        std::unique_ptr<TMessageType> getMessage(double timeout, const std::string &error_prefix = "") {
            auto id = ++messageCounter;
#ifndef NDEBUG
            char const *name = typeid(TMessageType).name();
            std::string tn = boost::core::demangle(name);
            this->templateWaitingLog(tn, id);
#endif


            auto message = this->getMessage(timeout, false);

            if (auto msg = dynamic_cast<TMessageType *>(message.get())) {
                message.release();
#ifndef NDEBUG
                this->templateWaitingLogUpdate(tn, id);
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
        std::unique_ptr<SimulationMessage> getMessage() {
            return getMessage(true);
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
        std::unique_ptr<SimulationMessage> getMessage(double timeout) {
            return this->getMessage(timeout, true);
        }
        void putMessage(SimulationMessage *m);
        void dputMessage(SimulationMessage *msg);
        std::shared_ptr<S4U_PendingCommunication> iputMessage(SimulationMessage *msg);
        std::shared_ptr<S4U_PendingCommunication> igetMessage();

        static unsigned long generateUniqueSequenceNumber();

        static S4U_Mailbox *getTemporaryMailbox();
        static void retireTemporaryMailbox(S4U_Mailbox *mailbox);

        static void createMailboxPool(unsigned long num_mailboxes);

//        static S4U_Mailbox *generateUniqueMailbox(const std::string &prefix);

        /**
         * @brief The mailbox pool size
         */
        static unsigned long mailbox_pool_size;

        /**
         * @brief The default control message size
         */
        static double default_control_message_size;

        /**
         * @brief The "not a mailbox" mailbox, to avoid getting answers back when asked
         *        to prove an "answer mailbox"
         */
        static S4U_Mailbox *NULL_MAILBOX;

        const std::string get_name() const {
            return this->name;
        }

        const char *get_cname() const {
            return this->name.c_str();
        }

    private:
        friend class S4U_Daemon;

        simgrid::s4u::Mailbox *s4u_mb;

        std::unique_ptr<SimulationMessage> getMessage(bool log);
        std::unique_ptr<SimulationMessage> getMessage(double timeout, bool log);

        void templateWaitingLog(std::string type, unsigned long long id);
        void templateWaitingLogUpdate(std::string type, unsigned long long id);

        static std::deque<S4U_Mailbox *> free_mailboxes;
        static std::set<S4U_Mailbox *> used_mailboxes;
        static std::deque<S4U_Mailbox *> mailboxes_to_drain;
        static unsigned long long messageCounter;

        std::string name;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_S4U_MAILBOX_H
