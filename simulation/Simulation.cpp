//
// Created by Henri Casanova on 2/21/17.
//

#include "Simulation.h"
#include "../exception/Exception.h"

#include <simgrid/msg.h>

namespace WRENCH {

		/**
		 * @brief Default constructor
		 */
		Simulation::Simulation() {
			this->platform = nullptr;
		}

		/**
		 * @brief Default destructor
		 */
		Simulation::~Simulation() {

		}

		/**
		 * @brief Simulation initialization method. This method has to be called first.
		 *
		 * @param argc is a pointer to the number of arguments passed to main()
		 * @param argv is the list of arguments passed to main()
		 */

		void Simulation::init(int *argc, char **argv) {
			MSG_init(argc, argv);
		}

		/**
 * @brief Simulation initialization method. This method has to be called first.
 *
 * @param argc is a pointer to the number of arguments passed to main()
 * @param argv is the list of arguments passed to main()
 */

		void Simulation::launch() {
			MSG_main();
		}

		/**
		 * @brief instantiate a simulated platform
		 *
		 * @param filename is the path to a SimGrid XML platform descritpion file
		 */
		void Simulation::createPlatform(std::string filename) {
			this->platform = std::make_shared<Platform>(filename);
		}

		/**
		 * @brief method to instantiate a sequential task executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createSequentialTaskExecutor(std::string hostname) {

			// Create the compute service
			std::shared_ptr<SequentialTaskExecutor> executor;
			try {
				executor = std::make_shared<SequentialTaskExecutor>(hostname);
			} catch (Exception e) {
				throw e;
			}

			// Add it to the list of Compute Services
			sequential_task_executors.push_back(executor);

		}


		void Simulation::createSimpleWMS(Workflow *w, std::string hostname) {

			// Create the WMS
			std::shared_ptr<SequentialRandomWMS> wms;

			try {
				wms = std::make_shared<SequentialRandomWMS>(this, w, hostname);
			} catch (Exception e) {
				throw e;
			}

			// Add it to the list of WMSes
			WMSes.push_back(wms);

		}

		unsigned long Simulation::getNumberSequentialTaskExecutors() {
			return sequential_task_executors.size();
		}

		std::shared_ptr<SequentialTaskExecutor> Simulation::getSomeSequentialTaskExecutor() {
			return sequential_task_executors[0];
		}


		void Simulation::shutdown() {

			for(std::shared_ptr<SequentialTaskExecutor> executor : this->sequential_task_executors) {
				executor->stop();
			}

		}


};
