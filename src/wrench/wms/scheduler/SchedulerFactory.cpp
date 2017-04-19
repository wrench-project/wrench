/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

		/**
		 * @brief Get a pointer to the SchedulerFactory
		 * @return  a pointer to the SchedulerFactory
		 */
		SchedulerFactory *SchedulerFactory::getInstance() {
			static SchedulerFactory fact;
			return &fact;
		}

		/**
		 * @brief Register a scheduler ID
		 * @param sched_id: the scheduler ID
		 * @param factory_method: the factory method
		 * @return the ID
		 */
		std::string SchedulerFactory::Register(std::string sched_id, t_pfFactory factory_method) {
			s_list[sched_id] = factory_method;
			return sched_id;
		}

		/**
		 * @brief Create a Scheduler instsance
		 * @param sched_id: the scheduler ID
		 * @return a unique pointer to a Scheduler object
		 */
		std::unique_ptr<Scheduler> SchedulerFactory::Create(std::string sched_id) {
			return s_list[sched_id]();
		}

		/**
		 * @brief Default constructor
		 */
		SchedulerFactory::SchedulerFactory() {}
}