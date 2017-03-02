/**
 *  @file    SequentialRandomWMS.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::SequentialRandomWMS class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::SequentialRandomWMS class implements a simple WMS abstraction
 *
 */

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "../WMS.h"
#include "SequentialRandomWMSDaemon.h"

namespace WRENCH {

		class Simulation;

		class SequentialRandomWMS : public WMS {


		public:
				SequentialRandomWMS(Simulation *s, Workflow *w, std::string hostname);

		private:
				std::unique_ptr<SequentialRandomWMSDaemon> wms_process;
		};
}


#endif //WRENCH_SIMPLEWMS_H
