/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIM4U_DAEMON_H
#define WRENCH_SIM4U_DAEMON_H

#include <string>

#include <simgrid/s4u.hpp>
#include <iostream>

namespace wrench {

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief A generic "running daemon that listens on a mailbox" abstraction
		 */
		class S4U_Daemon {

        class LifeSaver {
        public:
            explicit LifeSaver(std::shared_ptr<S4U_Daemon> &reference) : reference(reference) {}
            std::shared_ptr<S4U_Daemon> reference;
        };

		public:
				/** @brief The name of the daemon */
				std::string process_name;
				/** @brief The name of the daemon's mailbox */
				std::string mailbox_name;
				/** @brief The name of the host on which the daemon is running */
				std::string hostname;

				S4U_Daemon(std::string hostname, std::string process_name_prefix, std::string mailbox_prefix);
				S4U_Daemon(std::string hostname, std::string process_name_prefix);

				virtual ~S4U_Daemon();

				void startDaemon(bool daemonized);

        void createLifeSaver(std::shared_ptr<S4U_Daemon> reference);

				virtual void cleanup();

				virtual int main() = 0;
        void setTerminated();
				std::string getName();

        LifeSaver *life_saver = nullptr;
    protected:

        void killActor();
        void join_actor();

		private:
				bool terminated;
				simgrid::s4u::ActorPtr s4u_actor;

		};

		/***********************/
		/** \endcond           */
		/***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOX_H
