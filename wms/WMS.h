//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_WMS_H
#define SIMULATION_WMS_H

#include "../workflow/Workflow.h"
#include "../platform/Platform.h"

namespace WRENCH {

		class WMS {

		public:
				WMS(Platform *p, Workflow *w);
				virtual ~WMS();

		private:
				Platform *platform;
				Workflow *workflow;

		};

};


#endif //SIMULATION_WMS_H
