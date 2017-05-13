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
#include <services/Service.h>

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
		class ComputeService : public Service {

		public:

				void stop();

				void runJob(WorkflowJob *job);

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

				virtual void runStandardJob(StandardJob *job);

				virtual void runPilotJob(PilotJob *job);

				ComputeService(std::string service_name, std::string mailbox_name_prefix);


		protected:

				friend class Simulation;
				void setSimulation(Simulation *simulation);

				Simulation *simulation;  // pointer to the simulation object

				bool supports_pilot_jobs;
				bool supports_standard_jobs;

				/***********************/
				/** \endcond          **/
				/***********************/

		};

		/***********************/
		/** \endcond           */
		/***********************/

};



#endif //SIMULATION_COMPUTESERVICE_H
