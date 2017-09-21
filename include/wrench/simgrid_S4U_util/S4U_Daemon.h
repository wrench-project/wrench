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

namespace wrench {

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief A generic "running daemon that listens on a mailbox" abstraction
		 */
		class S4U_Daemon {

		public:
				/** @brief The name of the daemon */
				std::string process_name;
				/** @brief The name of the daemon's mailbox */
				std::string mailbox_name;
				/** @brief The name of the host on which the daemon is running */
				std::string hostname;

				S4U_Daemon(std::string process_name, std::string mailbox_prefix);
				S4U_Daemon(std::string process_name);

				void start(std::string hostname);
				/** @brief The daemon's main routine
				 * @return 0 on success, !=0 otherwise
				 */
				virtual int main() = 0;
        void setTerminated();
				std::string getName();

		protected:
				void kill_actor();

		private:
				bool terminated;
				simgrid::s4u::ActorPtr s4u_actor;


		};

		/***********************/
		/** \endcond           */
		/***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOX_H
