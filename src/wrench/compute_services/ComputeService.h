/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMULATION_COMPUTESERVICE_H
#define SIMULATION_COMPUTESERVICE_H

#include <map>
#include <workflow_job/WorkflowJob.h>

namespace wrench {

		/***********************/
		/** \cond DEVELOPER   **/
		/***********************/

		/* Forward References */

		class Simulation;

		class WorkflowJob;

		class StandardJob;

		class PilotJob;

		/**
		 * @brief Abstract implementation of a compute service.
		 */
		class ComputeService {

		public:



				virtual void stop();

				void runJob(WorkflowJob *job);

				std::string getName();

				bool isUp();

				bool canRunJob(WorkflowJob::Type job_type, unsigned long min_num_cores, double duration);

				bool supportsStandardJobs();

				bool supportsPilotJobs();

				virtual double getCoreFlopRate();

				virtual unsigned long getNumCores();

				virtual unsigned long getNumIdleCores();

				virtual double getTTL();



				/***********************/
				/** \cond INTERNAL    **/
				/***********************/

				enum State {
						UP,
						DOWN,
				};

				virtual void runStandardJob(StandardJob *job);

				virtual void runPilotJob(PilotJob *job);

				ComputeService(std::string service_name, Simulation *simulation);

				ComputeService(std::string service_name);

				void setSupportStandardJobs(bool v);

				void setSupportPilotJobs(bool v);

				void setStateToDown();

		protected:
				friend class Simulation;

				ComputeService::State state;
				std::string service_name;
				Simulation *simulation;  // pointer to the simulation object

		private:
				bool supports_pilot_jobs;
				bool supports_standard_jobs;

				/***********************/
				/** \endcond INTERNAL **/
				/***********************/

		};

		/***********************/
		/** \endcond DEVELOPER */
		/***********************/

};



#endif //SIMULATION_COMPUTESERVICE_H
