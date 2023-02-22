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
        template<class TMessageType>
        static std::unique_ptr<TMessageType> getMessage(simgrid::s4u::Mailbox *mailbox,const std::string& error_prefix=""){
            #ifndef NDEBUG
                char const *name = typeid(TMessageType).name();
                std::string tn= boost::core::demangle(name);
                templateWaitingLog(mailbox,tn);
            #endif


            auto message=S4U_Mailbox::getMessage(mailbox,nullptr);
            if(auto msg=dynamic_cast<TMessageType*>(message.get())){
                message.release();
                return std::unique_ptr<TMessageType>(msg);
            }else{
                char const *name = typeid(TMessageType).name();
                std::string tn= boost::core::demangle(name);
                throw std::runtime_error(error_prefix+"Unexpected [" + message->getName() + "] message while waiting for "+ tn.c_str());
            }
        }
        template<class TMessageType>
        static std::unique_ptr<TMessageType> getMessage(simgrid::s4u::Mailbox *mailbox, double timeout,const std::string& error_prefix=""){

            templateWaitingLog(mailbox,(TMessageType*)nullptr);

            auto message=S4U_Mailbox::getMessage(mailbox,timeout,nullptr);
            if(auto msg=dynamic_cast<TMessageType>(message.get())){
                message.release();
                return std::unique_ptr<TMessageType>(msg);
            }else{
                throw std::runtime_error(error_prefix+"Unexpected [" + message->getName() + "] message while waiting for "+ WRENCH_BOOST_DEMANGLE_TYPE((TMessageType)nullptr));
            }
        }

        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox,void* log=(void*)1);
        static std::unique_ptr<SimulationMessage> getMessage(simgrid::s4u::Mailbox *mailbox, double timeout,void* log=(void*)1);
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
        static std::deque<simgrid::s4u::Mailbox *> free_mailboxes;
        static std::set<simgrid::s4u::Mailbox *> used_mailboxes;
        static std::deque<simgrid::s4u::Mailbox *> mailboxes_to_drain;
        static void templateWaitingLog(const simgrid::s4u::Mailbox* mailbox ,std::string type);
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_S4U_MAILBOX_H
