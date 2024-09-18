/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_S4U_COMMPORT_H
#define WRENCH_S4U_COMMPORT_H


#include <string>
#include <map>
#include <set>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <simgrid/s4u.hpp>
#include <wrench/simulation/SimulationMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Daemon.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class S4U_PendingCommunication;

    /**
     * @brief Wrappers around S4U's communication methods
     */
    class S4U_CommPort {

    public:
        /**
	 * @brief Returns a message type name as a string
	 */
        template<class TMessageType>
        std::string get_type_name() {
            char const *type_name = typeid(TMessageType).name();
            return boost::core::demangle(type_name);
        }

        S4U_CommPort();

        ~S4U_CommPort();

        /**
         * @brief Synchronously receive a message from a commport_name
         *
         * @param error_prefix: any string you wish to prefix the error message with
         * @return the message, in a unique_ptr of the type specified.  Otherwise throws a runtime_error
         *
         * @throw std::shared_ptr<NetworkError>
         */
        template<class TMessageType>
        std::unique_ptr<TMessageType> getMessage(const std::string &error_prefix = "") {
            auto id = ++messageCounter;
#ifndef NDEBUG
            this->templateWaitingLog(get_type_name<TMessageType>(), id);
#endif

            auto message = this->getMessage(false);

            if (auto msg = dynamic_cast<TMessageType *>(message.get())) {
#ifndef NDEBUG
                this->templateWaitingLogUpdate(get_type_name<TMessageType>(), id);
#endif
                message.release();
                return std::unique_ptr<TMessageType>(msg);
            } else {
                std::cerr << "message = " << message.get() << "\n";
                throw std::runtime_error(error_prefix + " Unexpected [" + ((message.get() != nullptr) ? message->getName() : "null-message") + "] message while waiting for " +
                                         get_type_name<TMessageType>() + ". Request ID: " + std::to_string(id));
            }
        }

        /**
         * @brief Synchronously receive a message from a commport
         *
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
            auto tn = get_type_name<TMessageType>();
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
                throw std::runtime_error(error_prefix + " Unexpected [" + message->getName() + "] message while waiting for " +
                                         get_type_name<TMessageType>() + ". Request ID: " + std::to_string(id));
            }
        }

        /**
         * @brief Synchronously receive a message from a commport_name
         *
         * @return the message, or nullptr (in which case it's likely a brutal termination)
         *
         * @throw std::shared_ptr<NetworkError>
         */
        std::unique_ptr<SimulationMessage> getMessage() {
            return getMessage(true);
        }

        /**
         * @brief Synchronously receive a message from a commport_name, with a timeout
         *
         * @param timeout:  a timeout value in seconds (<0 means never timeout)
         * @return the message, or nullptr (in which case it's likely a brutal termination)
         *
         * @throw std::shared_ptr<NetworkError>
         */
        std::unique_ptr<SimulationMessage> getMessage(double timeout) {
            return this->getMessage(timeout, true);
        }

        void reset();
        void putMessage(SimulationMessage *m);
        void dputMessage(SimulationMessage *msg);
        std::shared_ptr<S4U_PendingCommunication> iputMessage(SimulationMessage *msg);
        std::shared_ptr<S4U_PendingCommunication> igetMessage();


        static unsigned long generateUniqueSequenceNumber();
        static S4U_CommPort *getTemporaryCommPort();
        static void retireTemporaryCommPort(S4U_CommPort *commport);
        static void createCommPortPool();

        /**
         * @brief The commport_name pool size
         */
        static unsigned long commport_pool_size;

        /**
         * @brief The default control message size
         */
        static double default_control_message_size;

        /**
         * @brief The "not a commport_name" commport_name, to avoid getting answers back when asked
         *        to prove an "answer commport_name"
         */
        static S4U_CommPort *NULL_COMMPORT;


        /**
	 * @brief Return the commport's name (as a C++ string)
	 * @return the commport's name
	 */
        [[nodiscard]] const std::string get_name() const {
            return this->name;
        }

        /**
	 * @brief Return the commport's name (as a C-style string)
	 * @return the commport's name
	 */
        [[nodiscard]] const char *get_cname() const {
            return this->name.c_str();
        }

    private:
        friend class S4U_Daemon;
        friend class S4U_PendingCommunication;
        friend class SimpleStorageServiceNonBufferized;
        SimulationMessage *msg_mb;
        SimulationMessage *msg_mq;

        simgrid::s4u::Mailbox *s4u_mb;
        simgrid::s4u::MessageQueue *s4u_mq;
        bool mb_comm_posted = false;
        simgrid::s4u::CommPtr mb_comm;
        bool mq_comm_posted = false;
        simgrid::s4u::MessPtr mq_comm;

        std::unique_ptr<SimulationMessage> getMessage(bool log);
        std::unique_ptr<SimulationMessage> getMessage(double timeout, bool log);


        void templateWaitingLog(const std::string &type, unsigned long long id);
        void templateWaitingLogUpdate(const std::string &type, unsigned long long id);

        /** Statics **/
        friend class S4U_Simulation;
        static std::vector<std::unique_ptr<S4U_CommPort>> all_commports;
        static std::deque<S4U_CommPort *> free_commports;
        static std::set<S4U_CommPort *> used_commports;
        static std::deque<S4U_CommPort *> commports_to_drain;
        static unsigned long long messageCounter;


        std::string name;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_S4U_COMMPORT_H
