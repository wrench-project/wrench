/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::ComputeService implements an abstract compute service.
 */


#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H


#include <map>
#include <workflow_job/WorkflowJob.h>

namespace wrench {

		class Simulation;
		class WorkflowJob;
		class StandardJob;
		class PilotJob;

		class ComputeService {

		public:

				enum State {
						UP,
						DOWN,
				};

				/** Constructors **/
				ComputeService(std::string, Simulation *simulation);
				ComputeService(std::string);

				/** Job execution **/
				void runJob(WorkflowJob *job);
				virtual void runStandardJob(StandardJob *job);
				virtual void runPilotJob(PilotJob *job);

				/** Information getting **/
				std::string getName();
				bool isUp();
				virtual double  getCoreFlopRate() = 0;
				virtual unsigned long getNumCores() = 0;
				virtual unsigned long getNumIdleCores() = 0;
				virtual double  getTTL() = 0;

				bool canRunJob(WorkflowJob::Type job_type, unsigned long min_num_cores, double duration);

				/** Stopping **/
				void setStateToDown();
				virtual void stop();

				/** Job types properties **/

				void setSupportStandardJobs(bool v);
				void setSupportPilotJobs(bool v);
				bool supportsStandardJobs();
				bool supportsPilotJobs();


		protected:
				friend class Simulation;

				ComputeService::State state;
				std::string service_name;
				Simulation *simulation;  // pointer to the simulation object

		private:
				bool supports_standard_jobs;
				bool supports_pilot_jobs;
		};
};


#endif //SIMULATION_COMPUTESERVICE_H
