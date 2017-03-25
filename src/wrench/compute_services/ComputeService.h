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

				enum Property {
						SUPPORTS_STANDARD_JOBS,
						SUPPORTS_PILOT_JOBS,
				};

				/** Constructors **/
				ComputeService(std::string, Simulation *simulation);
				ComputeService(std::string);

				/** Job execution **/
				void runJob(WorkflowJob *job);

				// Should be excluded from documentation
				virtual void runStandardJob(StandardJob *job);
				virtual void runPilotJob(PilotJob *job);

				/** Information getting **/
				virtual unsigned long numIdleCores() = 0;
				std::string getName();
				ComputeService::State getState();
				bool canRunJob(WorkflowJob*);

				/** Stopping **/
				virtual void stop();

				/** Getting properties **/
				bool hasProperty(ComputeService::Property);
				std::string getProperty(ComputeService::Property);


		protected:
				friend class Simulation;

				ComputeService::State state;
				std::string service_name;
				Simulation *simulation;  // pointer to the simulation object
				std::map<ComputeService::Property, std::string> property_list;

				void setProperty(ComputeService::Property, std::string value);
		};
};


#endif //SIMULATION_COMPUTESERVICE_H
