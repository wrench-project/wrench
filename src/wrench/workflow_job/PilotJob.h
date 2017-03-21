/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */

#ifndef WRENCH_PILOTJOB_H
#define WRENCH_PILOTJOB_H



namespace wrench {

		class ComputeService;

		class PilotJob {
		public:
				enum State {
						SUBMITTED,
						RUNNING,
						EXPIRING
				};

				PilotJob();
				PilotJob::State getState();
				ComputeService *getComputeService();
				void stop();

		private:
				State state;
				ComputeService *compute_service; // Associated compute service
		};

};


#endif //WRENCH_PILOTJOB_H
