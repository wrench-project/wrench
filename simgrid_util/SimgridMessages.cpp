//
// Created by Henri Casanova on 2/22/17.
//

#include "SimgridMessages.h"

namespace WRENCH {

		SimgridMessage::SimgridMessage(Type t, double f, double b) {
			type = t;
			flops = f;
			bytes = b;
		}

		SimgridMessage::~SimgridMessage() {
		}

		// TODO: Crate an infrastructure to set up values like the 0.0 and 1024 bytes below
		StopDaemonMessage::StopDaemonMessage(): SimgridMessage(STOP_DAEMON, 0.0, 1024) {
		}

		StopDaemonMessage::~StopDaemonMessage() {
		}
};
