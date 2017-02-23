//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "../workflow/Workflow.h"
#include "../platform/Platform.h"

namespace WRENCH {

		class Platform;
		class Workflow;

		class WMS {

		public:
				WMS(Workflow *w);
				virtual ~WMS();

		private:
				Workflow *workflow;

		};

};


#endif //WRENCH_WMS_H
