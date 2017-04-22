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


#include <simulation/SimulationMessage.h>

namespace wrench {

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief Wrappers around Simgrid's mailbox methods
		 */
		class S4U_Mailbox {

		public:
				static std::string generateUniqueMailboxName(std::string);
				static std::unique_ptr<SimulationMessage> get(std::string mailbox);
				static std::unique_ptr<SimulationMessage> get(std::string mailbox, double timeout);
				static void put(std::string mailbox, SimulationMessage *m);
				static void dput(std::string mailbox_name, SimulationMessage *msg);
				static void clear_dputs();


		private:
				static std::map<simgrid::s4u::ActorPtr , std::set<simgrid::s4u::Comm*>> dputs;

		};

		/***********************/
		/** \endcond           */
		/***********************/

};


#endif //WRENCH_S4U_MAILBOX_H
