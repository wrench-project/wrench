/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "wms/engine/EngineFactory.h"

namespace wrench {

		extern const char simplewms_name[] = "simple_wms";

		class Simulation; // forward ref

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 *  @brief A simple WMS abstraction
		 */
		class SimpleWMS : public EngineTmpl<simplewms_name, SimpleWMS> {

		public:
				SimpleWMS();

		private:
				int main();
		};

		/***********************/
		/** \endcond           */
		/***********************/

}
#endif //WRENCH_SIMPLEWMS_H
