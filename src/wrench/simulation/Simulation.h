//
// Created by Henri Casanova on 2/21/17.
//

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>
#include "../workflow/Workflow.h"
#include "../compute_services/sequential_task_executor/SequentialTaskExecutor.h"
#include "../wms/sequential_random_WMS/SequentialRandomWMS.h"
#include "../platform/Platform.h"


namespace WRENCH {


		class Simulation; // forward ref

		class Simulation  {

		public:
				Simulation();
				~Simulation();

				void init(int *, char **);
				void createPlatform(std::string);
				void createSequentialTaskExecutor(std::string hostname);
				void createSimpleWMS(Workflow *w, std::string hostname);

				std::shared_ptr<SequentialTaskExecutor> getSomeSequentialTaskExecutor();

				void launch();

				void shutdown();

		private:

				std::shared_ptr<Platform> platform;
				std::vector<std::shared_ptr<SequentialTaskExecutor>> sequential_task_executors;
				std::vector<std::shared_ptr<SequentialRandomWMS>> WMSes;

		};

};


#endif //WRENCH_SIMULATION_H
