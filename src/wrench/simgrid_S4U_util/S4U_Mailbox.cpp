/**
 * @brief S4U_Mailbox is a S4U wrapper
 */

#include <simulation/SimulationMessage.h>
#include <exception/WRENCHException.h>
#include "S4U_Mailbox.h"
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(mailbox, "Mailbox");

namespace WRENCH {

		/**
		 * @brief A blocking method to receive a message from a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @return a unique pointer to the message
		 */
		std::unique_ptr<SimulationMessage> S4U_Mailbox::get(std::string mailbox_name) {

		  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
		  SimulationMessage *msg = static_cast<SimulationMessage*>(simgrid::s4u::this_actor::recv(mailbox));
			if (msg == NULL) {
				throw WRENCHException("Mailbox::get(): NULL message in task");
			}

			return std::unique_ptr<SimulationMessage>(msg);
		}

		/**
		 * @brief A blocking method to send a message to a mailbox
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void S4U_Mailbox::put(std::string mailbox_name, SimulationMessage *msg) {

			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			simgrid::s4u::this_actor::send(mailbox, msg, (size_t)msg->size);

			return;
		}

		/**
		 * @brief A non-blocking method to send a message to a mailbox. This
		 *        is a "fire and forget" method, meaning that there is no
		 *        provided method to check that the put() has completed.
		 *
		 * @param mailbox is the mailbox name
		 * @param m is the message
		 */
		void S4U_Mailbox::iput(std::string mailbox_name, SimulationMessage *msg) {
			simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(mailbox_name);
			simgrid::s4u::Comm &comm = simgrid::s4u::Comm::send_async(mailbox, msg, (int)msg->size);
			//TODO: Can we do a dsend() so that we ignore the comm?
			return;
		}

};