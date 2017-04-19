/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_WMS_H
#define WRENCH_WMS_H

#include "workflow/Workflow.h"
#include "wms/scheduler/Scheduler.h"

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief Abstract implementation of a Workflow Management System
		 */
		class WMS {

		protected:
				WMS() {};
				std::string wms_type;

		public:
				virtual ~WMS() {};

				virtual void configure(Simulation *simulation,
															 Workflow *workflow,
															 std::unique_ptr<Scheduler> scheduler,
															 std::string hostname) = 0;
		};

		/***********************/
		/** \endcond DEVELOPER */
		/***********************/

};


#endif //WRENCH_WMS_H
