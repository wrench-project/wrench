/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief An abstract Job class
 */


#ifndef WRENCH_WORKFLOWJOB_H
#define WRENCH_WORKFLOWJOB_H


namespace wrench {

		class WorkflowJob {
		public:
				enum Type {
						STANDARD,
						PILOT
				};

				Type type;

		};

};


#endif //WRENCH_WORKFLOWJOB_H
