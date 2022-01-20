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
//				static void clear_dputs();

				static std::string generateUniqueMailboxName(std::string);
				static unsigned long generateUniqueSequenceNumber();

		private:

//				static std::map<simgrid::s4u::ActorPtr , std::set<simgrid::s4u::CommPtr>> dputs;

		};

		/***********************/
		/** \endcond           */
		/***********************/

};


#endif //WRENCH_S4U_MAILBOX_H
