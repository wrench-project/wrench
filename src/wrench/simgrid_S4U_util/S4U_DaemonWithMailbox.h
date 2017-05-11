/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIM4U_DAEMONWITHMAILBOX_H
#define WRENCH_SIM4U_DAEMONWITHMAILBOX_H

#include <string>
#include <simgrid/s4u.hpp>

namespace wrench {

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief A generic "running daemon that listens on a mailbox" abstraction
		 */
		class S4U_DaemonWithMailbox {

		public:
				std::string process_name;
				std::string mailbox_name;

				S4U_DaemonWithMailbox(std::string process_name, std::string mailbox_prefix);
				void start(std::string hostname);
				virtual int main() = 0;
				void kill_actor();
        void setTerminated();

		private:
        bool terminated;
				simgrid::s4u::ActorPtr s4u_actor;


		};

		/***********************/
		/** \endcond           */
		/***********************/
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOX_H
