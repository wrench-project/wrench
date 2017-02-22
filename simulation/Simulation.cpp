//
// Created by Henri Casanova on 2/21/17.
//

#include "Simulation.h"

#include <simgrid/msg.h>

namespace WRENCH {

		/**
		 * @brief Default constructor
		 */
		 Simulation::Simulation() {

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

};
