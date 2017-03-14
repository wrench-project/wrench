/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief WRENCH::WMS is a mostly abstract implementation of a WMS
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "workflow/Workflow.h"

namespace wrench {

		class WMS {

		public:
				WMS(Workflow *w);

		private:
				Workflow *workflow;

		};

};


#endif //WRENCH_WMS_H
