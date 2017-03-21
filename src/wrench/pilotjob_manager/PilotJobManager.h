/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief PilotJobManager implements a simple facility for managing pilot
 *        jobs submitted by a WMS to pilot-job-enabled compute services.
 */


#ifndef WRENCH_PILOTJOBMANAGER_H
#define WRENCH_PILOTJOBMANAGER_H

#include <vector>
#include "compute_services/ComputeService.h"

namespace wrench {

		class PilotJob;
		class PilotJobManagerDaemon;

		class PilotJobManager {
			public:
				PilotJobManager();
				std::vector<PilotJob*> get_running_pilot_jobs();
				std::vector<PilotJob*> get_submitted_pilot_jobs();

			private:
				std::unique_ptr<PilotJobManagerDaemon> daemon;
				void newSubmittedPilotjob(ComputeService *);

		};

};

#endif //WRENCH_PILOTJOBMANAGER_H
