//
// Created by Henri Casanova on 3/9/17.
//

#ifndef WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
#define WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H

#include <xbt.h>
#include <string>
#include <vector>
#include <iostream>

#include "S4U_DaemonWithMailbox.h"


namespace WRENCH {

		//class Sim4U_DaemonWithMailbox;

		class S4U_DaemonWithMailboxActor {

		public:

				explicit S4U_DaemonWithMailboxActor(S4U_DaemonWithMailbox *d) {
					this->daemon = d;
				}

				void operator()() {
					this->daemon->main();
				}

		private:
				S4U_DaemonWithMailbox *daemon;

		};
};


#endif //WRENCH_SIM4U_DAEMONWITHMAILBOXACTOR_H
