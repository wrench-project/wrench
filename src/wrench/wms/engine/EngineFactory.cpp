/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::EngineFactory is a factory class for WMS engines
 */

#include "wms/engine/EngineFactory.h"

namespace wrench {

		/****************************/
		/**	INTERNAL METHODS BELOW **/
		/****************************/

		/*! \cond INTERNAL */

		EngineFactory *EngineFactory::getInstance() {
			static EngineFactory fact;
			return &fact;
		}

		std::string EngineFactory::Register(std::string wms_id, t_pfFactory factory_method) {
			s_list[wms_id] = factory_method;
			return wms_id;
		}

		std::unique_ptr<WMS> EngineFactory::Create(std::string wms_id) {
			return s_list[wms_id]();
		}

		EngineFactory::EngineFactory() {}

		/*! \endcond */
}