//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMGRIDMESSAGES_H
#define WRENCH_SIMGRIDMESSAGES_H

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



#endif //WRENCH_SIMGRIDMESSAGES_H
