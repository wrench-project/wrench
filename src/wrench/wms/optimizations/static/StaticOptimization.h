/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::StaticOptimization is a template class for static optimization to the workflow
 */

#ifndef WRENCH_STATICOPTIMIZATION_H
#define WRENCH_STATICOPTIMIZATION_H

#include "workflow/Workflow.h"

namespace wrench {

	class StaticOptimization {
	public:
		virtual void process(Workflow *workflow) = 0;
	};
}

#endif //WRENCH_STATICOPTIMIZATION_H
