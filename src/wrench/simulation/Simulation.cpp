/**
 *  @file    Simulation.cpp
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::Simulation class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::Simulation class is a top-level class that keeps track of
 *  everything.
 *
 */

#include <compute_services/multicore_task_executor/MulticoreTaskExecutor.h>
#include <xbt/log.h>
#include "Simulation.h"
#include "exception/WRENCHException.h"
#include "simgrid_util/Simgrid.h"

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
			// Set the logging format
			xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");
			Simgrid::initialize(argc, argv);
		}

		/**
		 * @brief Simulation initialization method. This method has to be called first.
		 *
		 * @param argc is a pointer to the number of arguments passed to main()
		 * @param argv is the list of arguments passed to main()
		 */

		void Simulation::launch() {
			Simgrid::runSimulation();
		}

		/**
		 * @brief instantiate a simulated platform
		 *
		 * @param filename is the path to a SimGrid XML platform descritpion file
		 */
		void Simulation::createPlatform(std::string filename) {
			this->platform = std::unique_ptr<Platform>(new Platform(filename));
		}

		/**
		 * @brief method to instantiate a sequential task executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createSequentialTaskExecutor(std::string hostname) {

			// Create the compute service
			std::unique_ptr<SequentialTaskExecutor> executor;
			try {
				executor = std::unique_ptr<SequentialTaskExecutor>(new SequentialTaskExecutor(hostname));
			} catch (WRENCHException e) {
				throw e;
			}

			// Add it to the list of Compute Services
			sequential_task_executors.push_back(std::move(executor));

		}

		/**
		 * @brief method to instantiate a multicore task executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createMulticoreTaskExecutor(std::string hostname) {

			// Create the compute service
			std::unique_ptr<MulticoreTaskExecutor> executor;
			try {
				executor = std::unique_ptr<MulticoreTaskExecutor>(new MulticoreTaskExecutor(hostname));
			} catch (WRENCHException e) {
				throw e;
			}

			// Add it to the list of Compute Services
			multicore_task_executors.push_back(std::move(executor));

		}

		/**
		 * @brief method to instantiave a simple WMS on a host
		 *
		 * @param w is the workflow that the WMS will execute
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createSimpleWMS(Workflow *w, std::string hostname) {

			// Create the WMS
			std::unique_ptr<SequentialRandomWMS> wms;

			try {
				wms = std::unique_ptr<SequentialRandomWMS>(new SequentialRandomWMS(this, w, hostname));
			} catch (WRENCHException e) {
				throw e;
			}

			// Add it to the list of WMSes
			WMSes.push_back(std::move(wms));

		}

		/**
		 * @brief temporary debug method to get the first sequential task executor
		 *
		 * @return
		 */
		SequentialTaskExecutor *Simulation::getSomeSequentialTaskExecutor() {
			return this->sequential_task_executors[0].get();
		}

		/**
		 * @brief temporary debug method to get the first multicore task executor
		 *
		 * @return
		 */
		MulticoreTaskExecutor *Simulation::getSomeMulticoreTaskExecutor() {
			return this->multicore_task_executors[0].get();
		}

		/**
		 * @brief method to shutdown all running compute services on the platform
		 */
		void Simulation::shutdown() {

			for (int i=0; i < this->sequential_task_executors.size(); i++) {
				this->sequential_task_executors[i]->stop();
			}
			for (int i=0; i < this->multicore_task_executors.size(); i++) {
				this->multicore_task_executors[i]->stop();
			}

		}


};
