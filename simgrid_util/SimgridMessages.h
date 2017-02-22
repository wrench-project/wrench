//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_SIMGRIDMESSAGES_H
#define SIMULATION_SIMGRIDMESSAGES_H

#include <string>

namespace WRENCH {

		/* Base struct */
		struct SimgridMessage {

				/* Message type enum */
				enum Type {
						STOP_DAEMON,
				};

				SimgridMessage(Type t, double f, double b);
				virtual ~SimgridMessage();

				Type type;
				std::string name;
				double flops;
				double bytes;
		};

		struct StopDaemonMessage: public SimgridMessage {

				StopDaemonMessage();
				~StopDaemonMessage();
		};

};



#endif //SIMULATION_SIMGRIDMESSAGES_H
