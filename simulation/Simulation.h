//
// Created by Henri Casanova on 2/21/17.
//

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>
#include "../workflow/Workflow.h"
#include "../compute_services/sequential_task_executor/SequentialTaskExecutor.h"
#include "../wms/simple_WMS/SimpleWMS.h"
#include "../platform/Platform.h"


namespace WRENCH {

//		class Platform;
//		class SimpleWMS;
//		class SequentialTaskExecutor;
//		class Workflow;

		class Simulation;

	class Simulation  {

		public:
			Simulation();
			~Simulation();

			void init(int *, char **);
			void createPlatform(std::string);
			void createSequentialTaskExecutor(std::string hostname);
			void createSimpleWMS(Workflow *w, std::string hostname);

			unsigned long getNumberSequentialTaskExecutors();

			void launch();

			void shutdown();

	private:

			std::shared_ptr<Platform> platform;
			std::vector<std::shared_ptr<SequentialTaskExecutor>> compute_services;
			std::vector<std::shared_ptr<SimpleWMS>> WMSes;

	};

};


#endif //WRENCH_SIMULATION_H
